#include "world_effects.h"
#include <algorithm>
#include <utility>
#include "resources/resource_store.h"
#include "simulation/model_math.h"

namespace earthsiege {

live_explosion::live_explosion(int16_t const selected, explosion_state value, shape_part const &shape, shape_part const &secondary_shape)
  : definition{selected}, state{std::move(value)}, model{shape} {
  secondary_position = state.position;
  if(state.secondary_cell) secondary.emplace(secondary_shape);
  synchronize_cells();
}

void live_explosion::synchronize_cells() {
  std::ranges::copy(state.cells, model.cell_frames().begin());
  if(secondary) secondary->cell_frames().front() = state.secondary_cell.value();
}

live_debris::live_debris(debris_particle value, shape_part const &shape, bool const weapon) : state{std::move(value)}, model{shape}, detached_weapon{weapon} {}
live_smoke_puff::live_smoke_puff(smoke_puff_state value, shape_part const &shape) : state{std::move(value)}, model{shape} {}

world_effects::world_effects(resource_store const &resources, sound_sink sink)
  : sounds{std::move(sink)}, explosion_definitions{read_explosions(resources.read("simvol0", "dat/EXPLOS.DAT"))},
    debris_definitions{read_debris(resources.read("simvol0", "dat/DEBRIS.DAT"))},
    explosion_models{read_model(resources.read("simvol0", "dts/EXPLOS.DTS"))},
    debris_models{read_model(resources.read("simvol0", "dts/DEBRIS.DTS"))},
    flat_models{read_model(resources.read("simvol0", "dts/FLAT.DTS"))},
    smoke_models{read_model(resources.read("simvol0", "dts/SMOKE.DTS"))},
    detached_weapon_models{read_model(resources.read("simvol0", "dts/MECHWPN2.DTS"))},
    smoke_textures{read_bitmaps(resources.read("simvol0", "dba/SMOKE.DBA"))},
    weapon_textures{read_bitmaps(resources.read("simvol0", "dba/WPNTEX.DBA"))} {
  for(size_t bank{0}; bank != explosion_textures.size(); ++bank) explosion_textures[bank] = read_bitmaps(resources.read("simvol0", "dba/EXPLO" + std::to_string(bank) + ".DBA"));
}

std::optional<size_t> world_effects::add_explosion(int16_t const selected, vec3<int32_t> const &position, random_state &random) {
  if(explosions.available() == 0) return std::nullopt;                            // original allocation failure precedes its random draw
  auto const &definition{explosion_definitions.at(static_cast<size_t>(selected))};
  auto const &shape{std::get<shape_part>(explosion_models.roots.at(static_cast<size_t>(definition.model)).content)};
  auto const &secondary{std::get<shape_part>(flat_models.roots.at(1).content)};
  auto const slot{explosions.emplace(selected, create_explosion(definition, shape.cell_frame_counts.size(), position, random), shape, secondary)};
  auto &effect{explosions.at(*slot)};
  if(effect.state.light_intensity) {
    effect.light_slot = light_owners.emplace(*slot);
    if(!effect.light_slot) effect.state.light_intensity.reset();
    else {
      auto const source{lighting.add(position).value()};
      light_owners.at(*effect.light_slot) = source;
      lighting.intensity(source, *effect.state.light_intensity);
    }
  }
  if(sounds) sounds(sample_sound_request{sound_action::positioned,static_cast<uint16_t>(effect.state.sound),position});
  return slot;
}

void world_effects::add_debris(int16_t const group, vec3<int32_t> const &position, random_state &random) {
  for(auto &particle : create_debris_burst(debris_definitions.at(static_cast<size_t>(group)), position, random)) {
    auto const &shape{std::get<shape_part>(debris_models.roots.at(static_cast<size_t>(particle.model)).content)};
    // The original burst dereferences null after pool exhaustion. Retain its
    // bounded capacity and random draws, dropping excess visuals safely.
    debris.emplace(std::move(particle), shape);
  }
}

std::optional<size_t> world_effects::add_smoke(int16_t const family, std::function<vec3<int32_t>()> attachment, random_state &random) {
  if(!emitters.available()) return {};
  return emitters.emplace(live_smoke_emitter{create_smoke_emitter(family, random), std::move(attachment)});
}
std::optional<size_t> world_effects::add_puff(int16_t const root, vec3<int32_t> const &position) {
  auto const slot{puffs.next_free_slot()}; if(!slot) return {};
  auto const &shape{std::get<shape_part>(smoke_models.roots.at(static_cast<size_t>(root)).content)};
  return puffs.emplace(smoke_puff_state{root, vec3<int32_t>{position}, vec3<int16_t>{retained_puff_velocities[*slot]}}, shape);
}
std::optional<size_t> world_effects::add_detached_weapon(debris_particle state) {
  auto const &shape{std::get<shape_part>(detached_weapon_models.roots.at(static_cast<size_t>(state.model)).content)};
  return debris.emplace(std::move(state), shape, true);
}

void world_effects::advance(frame_ratio const ratio) {
  auto const &secondary{std::get<shape_part>(flat_models.roots.at(1).content)};
  explosions.for_each_oldest([&](size_t const slot, live_explosion &effect){
    if(explosions.retiring(slot)) return;
    auto const &definition{explosion_definitions.at(static_cast<size_t>(effect.definition))};
    auto const &shape{std::get<shape_part>(explosion_models.roots.at(static_cast<size_t>(definition.model)).content)};
    bool const done{advance_explosion(effect.state, definition, shape.cell_frame_counts, static_cast<uint16_t>(secondary.cell_frame_counts.front()), ratio)};
    effect.synchronize_cells();
    if(effect.light_slot) lighting.intensity(light_owners.at(*effect.light_slot), *effect.state.light_intensity);
    if(done) {
      if(effect.light_slot) light_owners.defer_erase(*effect.light_slot);
      explosions.defer_erase(slot);
    }
  });
  puffs.for_each_oldest([&](size_t const slot, live_smoke_puff &puff){
    if(puffs.retiring(slot)) return;
    auto const &shape{std::get<shape_part>(smoke_models.roots.at(static_cast<size_t>(puff.state.root)).content)};
    if(advance_smoke_puff(puff.state, static_cast<uint16_t>(shape.cell_frame_counts.front()), ratio)) puffs.defer_erase(slot);
    puff.model.cell_frames().front() = puff.state.cell;
  });
  emitters.for_each_oldest([&](size_t const slot, live_smoke_emitter &emitter){
    if(emitters.retiring(slot)) return;
    if(auto const root{advance_smoke_emitter(emitter.state, ratio)}) add_puff(*root, emitter.attachment());
    if(!emitter.state.remaining) emitters.defer_erase(slot);
  });
  debris.for_each_oldest([&](size_t const slot, live_debris &particle){
    if(!debris.retiring(slot) && advance_debris(particle.state)) debris.defer_erase(slot);
  });
}

void world_effects::collect() {
  light_owners.collect([&](size_t, size_t const source){ lighting.remove(source); });
  explosions.collect();
  debris.collect();
  puffs.collect([&](size_t const slot, live_smoke_puff const &puff){retained_puff_velocities[slot] = puff.state.velocity;});
  emitters.collect();
}

effect_lighting &world_effects::lights() { return lighting; }

void world_effects::append_models(std::vector<scene_model> &models) {
  explosions.for_each_oldest([&](size_t, live_explosion &effect){
    auto const &definition{explosion_definitions.at(static_cast<size_t>(effect.definition))};
    auto placement{rotation_transform(effect.state.angles)};
    placement.translation = effect.state.position;
    models.push_back({
      .roots{&explosion_models.roots.at(static_cast<size_t>(definition.model)), 1},
      .instance{effect.model}, .placement{placement}, .native_class{static_cast<int16_t>(definition.alternate_object_kind ? 8 : 2)},
      .textures{explosion_textures.at(static_cast<size_t>(definition.texture_bank))},
      .attachments{}, .detail_thresholds{}, .detail{},
    });
  });
  debris.for_each_oldest([&](size_t, live_debris &particle){
    auto placement{rotation_transform(particle.state.angles)};
    placement.translation = particle.state.position;
    models.push_back({
      .roots{&(particle.detached_weapon ? detached_weapon_models : debris_models).roots.at(static_cast<size_t>(particle.state.model)), 1},
      .instance{particle.model}, .placement{placement}, .native_class{1},
      .textures{particle.detached_weapon ? std::span<bitmap const>{weapon_textures} : std::span<bitmap const>{}}, .attachments{}, .detail_thresholds{}, .detail{},
    });
  });
  puffs.for_each_oldest([&](size_t, live_smoke_puff &puff){
    auto placement{rotation_transform({0,0,0})}; placement.translation = puff.state.position;
    models.push_back({.roots{&smoke_models.roots.at(static_cast<size_t>(puff.state.root)), 1},
      .instance{puff.model}, .placement{placement}, .native_class{4},
      .textures{smoke_textures}, .attachments{}, .detail_thresholds{}, .detail{}});
  });
}
std::vector<ground_flash> world_effects::ground_flashes() {
  std::vector<ground_flash> result;
  explosions.for_each_oldest([&](size_t, live_explosion &effect){
    if(effect.secondary) result.push_back({flat_models.roots.at(1), *effect.secondary, effect.secondary_position});
  });
  return result;
}

object_pool<live_smoke_puff> const &world_effects::live_smoke() const { return puffs; }
object_pool<live_smoke_emitter> const &world_effects::smoke_controllers() const { return emitters; }
object_pool<live_explosion> const &world_effects::live_explosions() const { return explosions; }
object_pool<live_debris> const &world_effects::live_particles() const { return debris; }

} // namespace earthsiege
