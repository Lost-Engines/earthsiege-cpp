#include "mission_session.h"
#include <optional>
#include <stdexcept>
#include <utility>
#include "resources/model_tree.h"
#include "resources/name_table.h"
#include "resources/resource_store.h"
#include "simulation/actor_placement.h"
#include "simulation/hit_dispatch.h"

namespace earthsiege {

mission_session::mission_session(resource_store const &resources, mission_script const &script, player_roster const &roster)
  : terrain{resources, script.header},
    weapon_definitions{read_weapon_catalogue(resources.read("simvol0", "dat/WEAPONS.DAT"))},
    weapon_models{read_model(resources.read("simvol0", "dts/MECHWPNS.DTS"))},
    weapon_textures{read_bitmaps(resources.read("simvol0", "dba/WPNTEX.DBA"))},
    effects{resources}, projectiles{resources, weapon_definitions} {
  auto const placements{placement_catalogue{resources}.place(script, roster)};
  auto const names{read_resource_names(resources.read("simvol0", "nam/MECHS.NAM"))};
  std::optional<size_t> controlled;
  for(auto const &actor : placements.actors[static_cast<size_t>(actor_kind::herc)]) {
    if(!actor.player_roster) continue;
    if(!actor.position) throw std::runtime_error{"player roster has an undefined formation position"};
    auto &definition{definitions[actor.definition]};
    if(!definition) definition = std::make_unique<herc_definition>(read_herc_definition(resources, names.at(static_cast<size_t>(actor.definition))));
    auto const &loadout{roster.hercs.at(actor.source - script.hercs.size())};
    crew_member member{
      .definition{definition.get()},
      .actor{std::make_unique<herc_actor>(*definition, weapon_definitions, weapon_models, loadout.weapons, *actor.position, actor.angles, loadout.weapon_values)},
      .attachments{},
    };
    for(auto const &part : member.actor->body().weapons().parts()) member.attachments.push_back({.body_part{part.body_part}, .geometry{part.geometry ? &*part.geometry : nullptr}, .cells{part.cell_frames}, .textures{weapon_textures}});
    if(actor.player_controlled) controlled = crew.size();
    crew.push_back(std::move(member));
  }
  if(!controlled) throw std::runtime_error{"mission has no controlled roster HERC"};
  player_index = *controlled;
  cycle_weapon();
}

scenery_world const &mission_session::scenery() const {
  return terrain;
}

herc_body const &mission_session::player() const {
  return crew.at(player_index).actor->body();
}

void mission_session::step(herc_controls const &controls, int16_t const time_ratio, bool const fire) {
  frame_contacts.clear();
  frame_notices.clear();
  effects.advance(time_ratio);
  projectiles.advance(time_ratio, [&](projectile_owner &owner){
    // Target acquisition/controller policy is still separate. The inspection
    // currently fires unguided; retained target handles are resolved here later.
    projectile_guidance input;
    input.player_owner = owner.actor == player_index;
    return input;
  }, [&](projectile_owner const &owner, impact_request const &request){ return impact(owner, request); });
  crew.at(player_index).actor->step(controls, time_ratio, [&](herc_body const &candidate){
    std::vector<contact_actor> bodies;
    bodies.reserve(crew.size());
    for(auto const &member : crew) bodies.push_back(member.actor->body().contact_geometry());
    if(auto const hit{find_actor_contact(bodies, player_index)}) {
      crew.at(hit->other).contact_received = true;
      frame_contacts.push_back(*hit);                                          // retain damage dispatch for the component-damage consumer
      return true;
    }
    return scenery_contact_height(terrain.colliders(), candidate.movement().position) != 0;
  });
  fire_selected(fire);
  for(auto &member : crew) if(!member.actor->damage().consequences.dead) member.actor->update_power(time_ratio);
  // Phase-five reclamation follows firing: expired objects occupy their slots
  // through this frame's simulation, as in the original shared pools.
  effects.collect();
  projectiles.collect();
}

bool mission_session::select_weapon(size_t const slot) {
  auto const bank{crew.at(player_index).actor->weapons().slots()};
  if(slot >= bank.size() || !bank[slot] || !bank[slot]->projectile) return false;
  selected_weapon = slot;
  return true;
}

void mission_session::cycle_weapon() {
  auto const count{crew.at(player_index).actor->weapons().slots().size()};
  for(size_t next{0}; next != count; ++next) if(select_weapon((selected_weapon.value_or(count - 1) + 1 + next) % count)) return;
  selected_weapon.reset();
}

std::optional<size_t> mission_session::weapon_selection() const { return selected_weapon; }
herc_actor const &mission_session::player_actor() const { return *crew.at(player_index).actor; }
world_effects &mission_session::visual_effects() { return effects; }
world_projectiles const &mission_session::ordnance() const { return projectiles; }
std::span<combat_notice const> mission_session::combat_notices() const { return frame_notices; }

void mission_session::fire_selected(bool const pressed) {
  auto &actor{*crew.at(player_index).actor};
  if(!selected_weapon || actor.damage().consequences.dead || actor.damage().consequences.disabled) return;
  auto &weapon{actor.weapons().at(*selected_weapon)};
  bool trigger{pressed};
  if(auto *elf{weapon.elf()}) trigger = poll_elf_trigger(weapon.controls(), *elf, pressed);
  if(!trigger || !weapon.ready(weapon_definitions.weapons.at(static_cast<size_t>(weapon.definition)))) return;
  vec2<int16_t> const aim{actor.body().elevation().angle, actor.body().torso().angle};
  for(auto const &shot : actor.discharge_slot(*selected_weapon, aim)) {
    projectiles.launch({player_index, std::nullopt}, shot, random, [&](projectile_owner const &owner, impact_request const &request){ return impact(owner, request); });
  }
}

std::optional<int32_t> mission_session::impact(projectile_owner const &owner, impact_request const &request) {
  std::vector<impact_actor> actors;
  std::vector<size_t> group;
  actors.reserve(crew.size()); group.reserve(crew.size());
  for(size_t index{0}; index != crew.size(); ++index) {
    auto &victim{*crew[index].actor};
    auto const &damage{victim.damage().consequences};
    group.push_back(index);
    actors.push_back({
      .position{vec3<int32_t>{victim.body().movement().position}}, .group{0}, .side{0},
      .waiting{false}, .dead{damage.dead}, .disabled{damage.disabled}, .inactive{false}, .quiet{false}, .hit_enemy{false}, .trigger{},
      .hit{[&, index](impact_request const &incoming) -> std::optional<int32_t> {
        herc_damage_events events;
        events.explosion = [&](int16_t const definition, vec3<int32_t> const &position, random_state &rng){ effects.add_explosion(definition, position, rng); };
        events.debris = [&](int16_t const definition, vec3<int32_t> const &position, random_state &rng){ effects.add_debris(definition, position, rng); };
        events.attached_smoke = [&](int16_t const part, vec3<int16_t> const &position){ frame_notices.push_back({combat_notice_kind::attached_smoke, index, part, 0, vec3<int32_t>{position}}); };
        events.weapon_ejected = [&](size_t const slot){ frame_notices.push_back({combat_notice_kind::ejected_weapon, index, static_cast<int16_t>(slot), 0, {}}); };
        events.salvage = [&](int16_t const weapon, int16_t const quality){ frame_notices.push_back({combat_notice_kind::salvage, index, weapon, quality, {}}); };
        events.shield_warning = [&]{ frame_notices.push_back({combat_notice_kind::shield_warning, index, 0, 0, {}}); };
        events.cockpit_damage = [&](size_t const part){ frame_notices.push_back({combat_notice_kind::cockpit_damage, index, static_cast<int16_t>(part), 0, {}}); };
        impact_conditions const conditions{.player{index == player_index}, .attacker_side{actors.at(owner.actor).side}, .difficulty{0}, .difficulty_override{false}, .side{actors[index].side}};
        auto const result{crew[index].actor->receive_impact(incoming, conditions, random, events)};
        auto const &state{crew[index].actor->damage().consequences};
        actors[index].dead = state.dead; actors[index].disabled = state.disabled;
        return result ? std::optional<int32_t>{result->distance} : std::nullopt;
      }},
    });
  }
  impact_world world{actors, terrain.colliders(), random, {}};
  world.events.effect = [&](vec3<int32_t> const &position, int16_t const definition){ effects.add_explosion(definition, position, random); };
  world.events.friendly_warning = [&](size_t const actor){ frame_notices.push_back({combat_notice_kind::friendly_warning, actor, 0, 0, {}}); };
  return dispatch_impact(request, {owner.actor, owner.actor == player_index, group, std::nullopt}, world);
}

std::vector<scene_object> mission_session::visible_objects() {
  auto models{roster_models()};
  effects.append_models(models);
  std::vector<scene_object> result;
  result.reserve(models.size() + projectiles.live().size());
  for(auto const &model : models) result.emplace_back(model);
  projectiles.append_objects(result);
  return result;
}

std::span<actor_contact const> mission_session::contacts() const { return frame_contacts; }

std::vector<scene_model> mission_session::roster_models() {
  std::vector<scene_model> result;
  result.reserve(crew.size());
  for(auto &member : crew) {
    size_t index{0};
    for(auto const &part : member.actor->body().weapons().parts()) member.attachments.at(index++).geometry = part.geometry && !part.disabled ? &*part.geometry : nullptr;
    result.push_back({
      .roots{member.definition->models.roots}, .instance{member.actor->body().model()},
      .placement{member.actor->body().placement()}, .native_class{7}, .textures{member.definition->textures},
      .attachments{member.attachments}, .detail_thresholds{herc_detail_thresholds}, .detail{},
    });
  }
  return result;
}

camera_target mission_session::view_target() const {
  auto const &member{crew.at(player_index)};
  camera_target result{.angles{vec3<uint16_t>{member.actor->body().movement().angles}}, .placement{member.actor->body().placement()}, .attachment{std::nullopt}};
  auto const part{find_model_part(member.definition->models.roots.front(), member.definition->profile.camera_part)};
  if(part) {
    if(part->header.transform < 0) throw std::runtime_error{"HERC camera part has no skeletal transform"};
    result.attachment = member.actor->body().model().transforms()[static_cast<size_t>(part->header.transform)];
  }
  return result;
}

camera_state mission_session::make_camera(camera_mode const mode) const {
  auto const &profile{crew.at(player_index).definition->profile};
  camera_state result;
  result.mode = mode;
  result.focus_offset = profile.camera_focus;
  result.attachment_offset = profile.camera_offset;
  update_camera(result, view_target());
  return result;
}

} // namespace earthsiege
