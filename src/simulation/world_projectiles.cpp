#include "world_projectiles.h"
#include <utility>
#include "resources/resource_store.h"

namespace earthsiege {

live_projectile::live_projectile(projectile_owner source, beam_effect beam)
  : owner{source}, state{std::in_place_type<beam_effect>, std::move(beam)} {}

live_projectile::live_projectile(projectile_owner source, weapon_shot const &shot, weapon_catalogue const &definitions, projectile_motions const &motions, model_resource const &bullets, model_resource const &rockets, int16_t const initial_rocket_clock)
  : owner{source}, state{std::in_place_type<projectile_instance>, shot, definitions, motions, bullets, rockets, source.target.has_value(), initial_rocket_clock} {}

world_projectiles::world_projectiles(resource_store const &resources, weapon_catalogue const &catalogue)
  : definitions{catalogue},
    motions{read_bullet_motions(resources.read("simvol0", "dat/BULLETS.DAT")), read_rocket_motions(resources.read("simvol0", "dat/ROCKETS.DAT"))},
    bullet_models{read_model(resources.read("simvol0", "dts/BULLETS.DTS"))},
    rocket_models{read_model(resources.read("simvol0", "dts/ROCKETS.DTS"))},
    bullet_textures{read_bitmaps(resources.read("simvol0", "dba/BULLETS.DBA"))} {}

std::optional<size_t> world_projectiles::launch(projectile_owner const owner, weapon_shot const &shot, random_state &random, world_impact_query const &hit) {
  if(shot.kind == shot_kind::beam) {
    // A beam applies its impact before trying to allocate its visible ribbon.
    auto const path{dispatch_beam(shot, definitions, [&](impact_request const &request){ return hit ? hit(owner, request) : std::nullopt; })};
    if(projectiles.available() == 0) return std::nullopt;
    auto const slot{projectiles.emplace(owner, make_beam_effect(definitions.projectiles.at(shot.projectile).subtype, path, random))};
    retained_rocket_clocks[*slot] = 0;
    return slot;
  }
  // The original bullet/rocket wrappers dereference a failed allocation. Keep
  // the shared limit, safely omitting the projectile when the pool is full.
  auto const slot{projectiles.next_free_slot()};
  if(!slot) return std::nullopt;
  return projectiles.emplace(owner, shot, definitions, motions, bullet_models, rocket_models, retained_rocket_clocks[*slot]);
}

void world_projectiles::advance(frame_ratio const ratio, projectile_guidance_query const &guidance, world_impact_query const &hit) {
  projectiles.for_each_oldest([&](size_t const slot, live_projectile &projectile){
    if(projectiles.retiring(slot)) return;
    bool done;
    if(auto *beam{std::get_if<beam_effect>(&projectile.state)}) done = advance_beam_effect(*beam, ratio);
    else {
      auto &flight{std::get<projectile_instance>(projectile.state)};
      auto input{guidance ? guidance(projectile.owner) : projectile_guidance{}};
      if(!projectile.owner.target) flight.clear_target();
      done = flight.advance(ratio, input, [&](impact_request const &request){ return hit ? hit(projectile.owner, request) : std::nullopt; }) != projectile_result::flying;
    }
    if(done) projectiles.defer_erase(slot);
  });
}

void world_projectiles::collect() {
  projectiles.collect([&](size_t const slot, live_projectile const &projectile){
    if(auto const *flight{std::get_if<projectile_instance>(&projectile.state)}; flight && flight->flight().kind == projectile_kind::rocket) retained_rocket_clocks[slot] = flight->flight().animation_remaining;
  });
}
object_pool<live_projectile> const &world_projectiles::live() const { return projectiles; }

void world_projectiles::append_objects(std::vector<scene_object> &objects) {
  projectiles.for_each_oldest([&](size_t, live_projectile const &projectile){
    if(auto const *beam{std::get_if<beam_effect>(&projectile.state)}) objects.emplace_back(scene_ribbon{vec3<int32_t>{beam->position}, beam->points, beam->faces, beam->colour});
    else {
      auto const &flight{std::get<projectile_instance>(projectile.state)};
      objects.emplace_back(scene_model{
        .roots{&flight.geometry(), 1}, .instance{flight.model()}, .placement{flight.placement()}, .native_class{3},
        .textures{flight.flight().kind == projectile_kind::bullet ? std::span<bitmap const>{bullet_textures} : std::span<bitmap const>{}},
        .attachments{}, .detail_thresholds{}, .detail{},
      });
    }
  });
}

} // namespace earthsiege
