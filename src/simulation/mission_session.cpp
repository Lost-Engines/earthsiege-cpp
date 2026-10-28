#include "mission_session.h"
#include <optional>
#include <stdexcept>
#include <utility>
#include "resources/model_tree.h"
#include "resources/name_table.h"
#include "resources/resource_store.h"
#include "simulation/actor_placement.h"
#include "simulation/hit_dispatch.h"
#include "simulation/flyer_navigation.h"
#include "simulation/model_math.h"
#include "simulation/fixed_math.h"

namespace earthsiege {

mission_session::mission_session(resource_store const &resources, mission_script const &script, player_roster const &roster, random_state initial_random, mission_counters initial_counters)
  : settings{script.header}, layout{resources}, routes{script.routes}, points{script.points}, terrain{resources, script.header}, ground_definitions{resources},
    weapon_definitions{read_weapon_catalogue(resources.read("simvol0", "dat/WEAPONS.DAT"))},
    weapon_models{read_model(resources.read("simvol0", "dts/MECHWPNS.DTS"))},
    weapon_textures{read_bitmaps(resources.read("simvol0", "dba/WPNTEX.DBA"))},
    random{std::move(initial_random)}, effects{resources}, projectiles{resources, weapon_definitions} {
  auto const placements{layout.place(script, roster)};
  activity=std::make_unique<mission_activity>(script,placements,initial_counters);
  auto const names{read_resource_names(resources.read("simvol0", "nam/MECHS.NAM"))};
  std::optional<size_t> controlled;
  for(auto const &actor : placements.actors[static_cast<size_t>(actor_kind::herc)]) {
    if(!actor.position) throw std::runtime_error{"mission contains an undefined negative-formation position"};
    auto &definition{definitions[actor.definition]};
    if(!definition) definition = std::make_unique<herc_definition>(read_herc_definition(resources, names.at(static_cast<size_t>(actor.definition))));
    std::span<int16_t const> equipment, values;
    vec3<int32_t> position{0, 0, -1};
    vec3<uint16_t> angles{32768, 32768, 0};
    if(actor.player_roster) {
      auto const &loadout{roster.hercs.at(actor.source - script.hercs.size())};
      equipment = loadout.weapons; values = loadout.weapon_values;
    } else {
      auto const &spawn{script.hercs.at(actor.source)};
      equipment = spawn.weapons; values = spawn.ammunition;
      if(spawn.position >= 0) { position = script.points.at(static_cast<size_t>(spawn.position)); position.z += definition->profile.position_height; }
      if(spawn.heading >= 0) angles = vec3<uint16_t>{0, 0, decode_mission_heading(script.headings.at(static_cast<size_t>(spawn.heading)))};
    }
    // Starting damage executes before group placement, including effects at an
    // unresolved actor's original sentinel position. Place the complete roster later.
    crew_member member{
      .definition{definition.get()},
      .actor{std::make_unique<herc_actor>(*definition, weapon_definitions, weapon_models, equipment, position, angles, values)},
      .attachments{}, .source{actor.source}, .group{actor.group}, .side{placements.groups.at(actor.group).mode},
      .waiting{script.groups.at(actor.group).trigger >= 0}, .player_roster{actor.player_roster},
      .requested_speed{!actor.player_roster && script.hercs.at(actor.source).field_mask[1] >= 0 ? script.hercs.at(actor.source).field_mask[1] : int16_t{0}},
    };
    for(auto const &part : member.actor->body().weapons().parts()) member.attachments.push_back({.body_part{part.body_part}, .geometry{part.geometry ? &*part.geometry : nullptr}, .cells{part.cell_frames}, .textures{weapon_textures}});
    if(actor.player_controlled) controlled = crew.size();
    crew.push_back(std::move(member));
    if(actor.player_roster) crew.back().actor->restore_condition(roster.hercs.at(actor.source - script.hercs.size()).condition);
    else crew.back().actor->apply_starting_condition(script.hercs.at(actor.source).initial_condition, random, damage_events(crew.size() - 1));
  }
  if(!controlled) throw std::runtime_error{"mission has no controlled roster HERC"};
  player_index = *controlled;
  for(size_t index{0}; index != crew.size(); ++index) {
    auto const &placement{placements.actors[static_cast<size_t>(actor_kind::herc)][index]};
    crew[index].actor->place(*placement.position, placement.angles);
  }
  auto const flyer_names{read_resource_names(resources.read("simvol0", "nam/FLYERS.NAM"))};
  for(auto const &placement : placements.actors[static_cast<size_t>(actor_kind::flyer)]) {
    if(!placement.position) throw std::runtime_error{"flyer has an undefined negative-formation position"};
    auto &definition{flyer_definitions[placement.definition]};
    if(!definition) definition = std::make_unique<flyer_definition>(read_flyer_definition(resources,flyer_names.at(static_cast<size_t>(placement.definition))));
    auto const side{placements.groups.at(placement.group).mode};
    flyers.push_back({std::make_unique<flyer_actor>(*definition,*placement.position,placement.angles),{},false,placement.source,placement.group,side,script.groups.at(placement.group).trigger >= 0});
  }
  for(auto const &placement : placements.actors[static_cast<size_t>(actor_kind::base)]) {
    if(!placement.position) throw std::runtime_error{"base has an undefined negative-formation position"};
    auto const side{placements.groups.at(placement.group).mode};
    auto const condition{script.bases.at(placement.source).initial_value};
    bases.push_back({std::make_unique<ground_actor>(ground_definitions, static_cast<size_t>(placement.definition), side, condition, *placement.position, placement.angles), placement.source, placement.group, side, script.groups.at(placement.group).trigger >= 0});
  }
  for(size_t i{0}; i != crew.size(); ++i) {
    auto const &condition{crew[i].actor->damage().consequences};
    auto &decision{activity->decisions()[i]}; decision.dead = condition.dead; decision.disabled = condition.disabled;
    crew[i].actor->notify_damage_changes([this, i](damage_consequences const &before, damage_consequences const &after){damage_consequences_changed(i, before, after);});
  }
  cycle_weapon();
}

mission_counters const &mission_session::mission_variables() const { return activity->variables(); }
mission_activity const &mission_session::activity_state() const { return *activity; }

scenery_world const &mission_session::scenery() const {
  return terrain;
}

herc_body const &mission_session::player() const {
  return crew.at(player_index).actor->body();
}

void mission_session::step(herc_controls const &controls, int16_t const time_ratio, bool const fire) {
  frame_contacts.clear();
  frame_notices.clear();
  for(auto &member : crew) member.actor->begin_frame();
  for(size_t i{0};i!=crew.size();++i)crew[i].waiting=activity->waiting(i);
  for(size_t i{0};i!=flyers.size();++i)flyers[i].waiting=activity->waiting(flyer_id(i));
  for(size_t i{0};i!=bases.size();++i)bases[i].waiting=activity->waiting(base_id(i));
  for(size_t index{0}; index != bases.size(); ++index) if(!bases[index].waiting) advance_base(index, time_ratio);
  effects.advance(time_ratio);
  projectiles.advance(time_ratio, [&](projectile_owner &owner){
    projectile_guidance input;
    input.player_owner = owner.actor == player_index;
    input.owner_tracks_target = activity->targets()[owner.actor].selected == owner.target;
    if(owner.target) {
      input.target_position = target_position(*owner.target, owner.target_component);
      input.target_detectable = observe().at(*owner.target).detectable;
    }
    return input;
  }, [&](projectile_owner const &owner, impact_request const &request){ return impact(owner, request); });
  activity->dispatch_groups([&](size_t const actor){
    if(actor < crew.size()) dispatch_herc(actor, time_ratio);
    else if(actor < base_id(0)) dispatch_flyer(actor - flyer_id(0), time_ratio);
  });
  crew.at(player_index).actor->prepare_player(controls, time_ratio, [&](movement_event event){movement_event_received(player_index, event);});
  fire_selected(fire);
  for(size_t i{0};i!=crew.size();++i)crew[i].waiting=activity->waiting(i);
  for(size_t i{0};i!=flyers.size();++i)flyers[i].waiting=activity->waiting(flyer_id(i));
  for(size_t i{0};i!=bases.size();++i)bases[i].waiting=activity->waiting(base_id(i));
  auto const observations{observe()};
  activity->scan_areas(observations);
  activity->perceive(observations,time_ratio,random,[&](size_t first,size_t second){return !intersect_scenery_segment(terrain.colliders(),observations[first].position,observations[second].position);});
  for(size_t i{0}; i != crew.size(); ++i) if(!crew[i].waiting && !crew[i].actor->damage().consequences.dead) {
    crew[i].actor->update_power(time_ratio, settings.free_weapons());
    update_herc_sensors(i, time_ratio);
  }
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
  for(auto const &shot : actor.discharge_slot(*selected_weapon, aim, settings.free_weapons())) {
    projectiles.launch({player_index, std::nullopt}, shot, random, [&](projectile_owner const &owner, impact_request const &request){ return impact(owner, request); });
  }
}

herc_damage_events mission_session::damage_events(size_t const index) {
  herc_damage_events events;
  events.explosion = [&, index](int16_t const definition, vec3<int32_t> const &position, random_state &rng){ effects.add_explosion(definition, position, rng); };
  events.debris = [&, index](int16_t const definition, vec3<int32_t> const &position, random_state &rng){ effects.add_debris(definition, position, rng); };
  events.attached_smoke = [&, index](int16_t const part, vec3<int16_t> const &position){ frame_notices.push_back({combat_notice_kind::attached_smoke, index, part, 0, vec3<int32_t>{position}}); };
  events.weapon_ejected = [&, index](size_t const slot){ frame_notices.push_back({combat_notice_kind::ejected_weapon, index, static_cast<int16_t>(slot), 0, {}}); };
  events.salvage = [&, index](int16_t const weapon, int16_t const quality){ frame_notices.push_back({combat_notice_kind::salvage, index, weapon, quality, {}}); };
  events.shield_warning = [&, index]{ frame_notices.push_back({combat_notice_kind::shield_warning, index, 0, 0, {}}); };
  events.cockpit_damage = [&, index](size_t const part){ frame_notices.push_back({combat_notice_kind::cockpit_damage, index, static_cast<int16_t>(part), 0, {}}); };
  return events;
}

ground_damage_events mission_session::base_damage_events(size_t const index) {
  ground_damage_events events;
  events.explosion = [&](int16_t const definition, vec3<int32_t> const &position, random_state &rng){ effects.add_explosion(definition, position, rng); };
  events.debris = [&](int16_t const definition, vec3<int32_t> const &position, random_state &rng){ effects.add_debris(definition, position, rng); };
  events.attached_smoke = [&, index](vec3<int16_t> const &position){ frame_notices.push_back({combat_notice_kind::attached_smoke, base_id(index), -1, 1, vec3<int32_t>{position}}); };
  events.destroyed = [&, index] {
    auto const id{base_id(index)};
    activity->notify_destroyed(id, observe());
    if(auto const trigger{activity->actor(id).hit_trigger}) activity->activate(*trigger);
  };

  return events;
}

size_t mission_session::flyer_id(size_t const index) const { return crew.size() + index; }
size_t mission_session::base_id(size_t const index) const { return crew.size() + flyers.size() + index; }
size_t mission_session::actor_group(size_t const index) const {
  if(index < crew.size()) return crew[index].group;
  if(index < base_id(0)) return flyers[index - flyer_id(0)].group;
  return bases.at(index - base_id(0)).group;
}
flyer_damage_events mission_session::flyer_events(size_t const index) {
  flyer_damage_events events;
  events.explosion = [&](int16_t const definition, vec3<int32_t> const &position, random_state &rng){ effects.add_explosion(definition,position,rng); };
  events.debris = [&](int16_t const definition, vec3<int32_t> const &position, random_state &rng){ effects.add_debris(definition,position,rng); };
  events.destroyed = [&, index] {
    auto const id{flyer_id(index)};
    activity->notify_destroyed(id, observe());
    if(auto const trigger{activity->actor(id).hit_trigger}) activity->activate(*trigger);
    enter_behavior(activity->decisions()[id].controller, behavior::flyer_dead, activity->entry_stagger());
  };

  return events;
}

std::optional<int32_t> mission_session::impact(projectile_owner const &owner, impact_request const &request) {
  std::vector<impact_actor> actors;
  std::vector<size_t> group;
  actors.reserve(base_id(bases.size())); group.reserve(base_id(bases.size()));
  for(size_t index{0}; index != crew.size(); ++index) {
    auto &victim{*crew[index].actor};
    auto const &damage{victim.damage().consequences};
    if(crew[index].group == actor_group(owner.actor)) group.push_back(index);
    actors.push_back({
      .position{vec3<int32_t>{victim.body().movement().position}}, .group{crew[index].group}, .side{crew[index].side},
      .waiting{crew[index].waiting}, .dead{damage.dead}, .disabled{damage.disabled}, .inactive{activity->decisions()[index].unavailable}, .quiet{activity->actor(index).awareness.event_busy}, .hit_enemy{false}, .trigger{activity->actor(index).contact_trigger},
      .hit{[&, index](impact_request const &incoming) -> std::optional<int32_t> {
        auto const events{damage_events(index)};
        impact_conditions const conditions{.player{index == player_index}, .attacker_side{actors.at(owner.actor).side}, .difficulty{settings.difficulty}, .difficulty_override{settings.campaign_mode != 0}, .side{actors[index].side}, .invulnerable{settings.player_is_invulnerable()}};
        auto const result{crew[index].actor->receive_impact(incoming, conditions, random, events)};
        auto const &state{crew[index].actor->damage().consequences};
        actors[index].dead = state.dead; actors[index].disabled = state.disabled;
        return result ? std::optional<int32_t>{result->distance} : std::nullopt;
      }},
    });
  }
  for(size_t index{0}; index != flyers.size(); ++index) {
    auto const &member{flyers[index]};
    if(member.group == actor_group(owner.actor)) group.push_back(flyer_id(index));
    actors.push_back({
      .position{vec3<int32_t>{member.actor->placement().translation}}, .group{member.group}, .side{member.side},
      .waiting{member.waiting}, .dead{member.actor->dead()}, .disabled{false}, .inactive{member.actor->dead()}, .quiet{activity->actor(flyer_id(index)).awareness.event_busy}, .hit_enemy{false}, .trigger{activity->actor(flyer_id(index)).contact_trigger},
      .hit{[&, index](impact_request const &incoming) -> std::optional<int32_t> {
        impact_conditions const conditions{.attacker_side{actors.at(owner.actor).side}, .difficulty{settings.difficulty}, .difficulty_override{settings.campaign_mode != 0}, .side{flyers[index].side}};
        auto const result{flyers[index].actor->receive_impact(incoming,conditions,random,flyer_events(index),owner.actor == player_index && owner.target == flyer_id(index))};
        actors[flyer_id(index)].dead = flyers[index].actor->dead();
        return result;
      }},
    });
  }
  for(size_t index{0}; index != bases.size(); ++index) {
    auto const &member{bases[index]};
    if(member.group == actor_group(owner.actor)) group.push_back(base_id(index));
    actors.push_back({
      .position{vec3<int32_t>{member.actor->placement().translation}}, .group{member.group}, .side{member.side},
      .waiting{member.waiting}, .dead{member.actor->dead()}, .disabled{false}, .inactive{false}, .quiet{activity->actor(base_id(index)).awareness.event_busy}, .hit_enemy{false}, .trigger{activity->actor(base_id(index)).contact_trigger},
      .hit{[&, index](impact_request const &incoming) -> std::optional<int32_t> {
        impact_conditions const conditions{.attacker_side{actors.at(owner.actor).side}, .difficulty{settings.difficulty}, .difficulty_override{settings.campaign_mode != 0}, .side{bases[index].side}};
        auto const result{bases[index].actor->receive_impact(incoming, conditions, random, base_damage_events(index))};
        actors[base_id(index)].dead = bases[index].actor->dead();
        return result;
      }},
    });
  }
  impact_world world{actors, terrain.colliders(), random, {}};
  world.events.trigger = [&](size_t const index){ activity->activate(index); };
  world.events.effect = [&](vec3<int32_t> const &position, int16_t const definition){ effects.add_explosion(definition, position, random); };
  world.events.friendly_warning = [&](size_t actor){friendly_fire_warning(actor);};
  world.events.react = [&](size_t victim, size_t attacker, int16_t damage){hit_reaction(victim, attacker, damage);};
  world.events.obstructed = [&](impact_obstruction const obstacle){
    if(owner.actor < crew.size()) {crew[owner.actor].obstruction = obstacle; crew[owner.actor].obstructed = true;}
  };
  std::optional<vec3<int32_t>> target;
  if(auto const selected{activity->targets()[owner.actor].selected}) target = combat_position(*selected).position;
  auto const result{dispatch_impact(request, {owner.actor, owner.actor == player_index, group, target}, world)};
  if(owner.actor < crew.size() && actors[owner.actor].hit_enemy) crew[owner.actor].hit_enemy = true;
  return result;
}

std::vector<scene_object> mission_session::visible_objects() {
  auto models{herc_models(false)};
  for(auto const &member : flyers) if(!member.waiting && !member.actor->dead()) models.push_back(member.actor->model());
  for(auto const &member : bases) if(!member.waiting) models.push_back(member.actor->model());
  effects.append_models(models);
  std::vector<scene_object> result;
  result.reserve(models.size() + projectiles.live().size());
  for(auto const &model : models) result.emplace_back(model);
  projectiles.append_objects(result);
  return result;
}

std::span<actor_contact const> mission_session::contacts() const { return frame_contacts; }

std::vector<scene_model> mission_session::roster_models() { return herc_models(true); }

std::vector<mission_herc_view> mission_session::hercs() const {
  std::vector<mission_herc_view> result;
  for(size_t index{0}; index != crew.size(); ++index) {
    auto const &member{crew[index]};
    result.push_back({member.source, member.group, member.side, member.waiting, member.player_roster, index == player_index, *member.actor});
  }
  return result;
}

std::vector<mission_base_view> mission_session::ground_actors() const {
  std::vector<mission_base_view> result;
  for(auto const &member : bases) result.push_back({member.source, member.group, member.side, member.waiting, *member.actor});
  return result;
}

std::vector<mission_flyer_view> mission_session::flying_actors() const {
  std::vector<mission_flyer_view> result;
  for(auto const &member : flyers) result.push_back({member.source,member.group,member.side,member.waiting,*member.actor});
  return result;
}

random_state::state const &mission_session::random_snapshot() const { return random.snapshot(); }

std::vector<scene_model> mission_session::herc_models(bool const roster_only) {
  std::vector<scene_model> result;
  result.reserve(crew.size());
  for(auto &member : crew) {
    if(roster_only ? !member.player_roster : member.waiting) continue;
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
