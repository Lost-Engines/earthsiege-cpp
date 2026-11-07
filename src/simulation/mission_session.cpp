#include "mission_session.h"
#include "simulation/impact_damage.h"
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
#include "simulation/player_targeting.h"
#include "simulation/mission_salvage.h"

namespace earthsiege {

mission_session::mission_session(resource_store const &resources, mission_script const &script, player_roster const &roster, random_state initial_random, mission_counters initial_counters)
  : settings{script.header}, layout{resources}, routes{script.routes}, points{script.points}, terrain{resources, script.header}, ground_definitions{resources},
    weapon_definitions{read_weapon_catalogue(resources)},
    weapon_models{read_model(resources.read("simvol0", "dts/MECHWPNS.DTS"))},
    weapon_textures{read_bitmaps(resources.read("simvol0", "dba/WPNTEX.DBA"))},
    random{std::move(initial_random)}, effects{resources,[this](auto const &event){emit_sound(event);},terrain.environment().atmosphere_parameters[1]!=0}, projectiles{resources, weapon_definitions,[this](auto const &event){emit_sound(event);}} {
  auto const placements{layout.place(script, roster, true)};
  activity=std::make_unique<mission_activity>(script,placements,initial_counters,false,resources.edition()==game_edition::expansion);
  for(auto const &group:script.groups)deployment_modes.push_back(resources.edition()==game_edition::expansion&&group.trigger>=0?script.triggers.at(static_cast<size_t>(group.trigger)).test:0);
  deployment_started.resize(script.groups.size());
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
      .actor{std::make_unique<herc_actor>(*definition, weapon_definitions, weapon_models, equipment, position, angles, values, actor.player_controlled ? sound_sink{[this](auto const &event){emit_sound(event);}} : sound_sink{})},
      .attachments{}, .source{actor.source}, .group{actor.group}, .side{placements.groups.at(actor.group).mode},
      .waiting{script.groups.at(actor.group).trigger >= 0}, .player_roster{actor.player_roster},
      .pilot_slot{actor.player_roster ? std::optional<int16_t>{roster.hercs.at(actor.source-script.hercs.size()).slot} : std::nullopt},
      .requested_speed{!actor.player_roster && script.hercs.at(actor.source).field_mask[1] >= 0 ? script.hercs.at(actor.source).field_mask[1] : int16_t{0}},
    };
    for(auto const &part : member.actor->body().weapons().parts()) member.attachments.push_back({.body_part{part.body_part}, .geometry{part.geometry ? &*part.geometry : nullptr}, .cells{part.cell_frames}, .textures{weapon_textures}});
    if(actor.player_controlled) controlled = crew.size();
    crew.push_back(std::move(member));
    auto const id{crew.size()-1};
    // Interleave initial controller entry and damage transitions exactly as the
    // original per-actor constructors do; each entry advances a shared stagger.
    enter_behavior(activity->decisions()[id].controller,behavior::herc_deciding,activity->entry_stagger());
    crew.back().actor->notify_damage_changes([this,id](auto const &before,auto const &after){
      damage_consequences_changed(id,before,after,true);
    });
    if(actor.player_roster) crew.back().actor->restore_condition(roster.hercs.at(actor.source - script.hercs.size()).condition);
    else crew.back().actor->apply_starting_condition(script.hercs.at(actor.source).initial_condition, random, damage_events(crew.size() - 1));
  }
  if(!controlled) throw std::runtime_error{"mission has no controlled roster HERC"};
  player_index = *controlled;
  player_chassis_id = placements.actors[static_cast<size_t>(actor_kind::herc)][player_index].definition;
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
    enter_behavior(activity->decisions()[flyer_id(flyers.size())].controller,behavior::flyer_deciding,activity->entry_stagger());
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
  player_weapons = std::make_unique<player_armament>(crew[player_index].actor->weapons(), weapon_definitions, crew[player_index].definition->mounts);
}

mission_counters const &mission_session::mission_variables() const { return activity->variables(); }
mission_activity const &mission_session::activity_state() const { return *activity; }

scenery_world const &mission_session::scenery() const {
  return terrain;
}

int16_t mission_session::player_chassis() const { return player_chassis_id; }

herc_body const &mission_session::player() const {
  return crew.at(player_index).actor->body();
}

void mission_session::step(herc_controls const &controls, int16_t const time_ratio, bool const fire) {
  player_input input{.movement{controls.steering, controls.throttle}, .aiming{controls.torso, controls.elevation}, .fire{fire}};
  advance_frame(input, time_ratio, [&]{
    crew.at(player_index).actor->prepare_player(controls, time_ratio, [&](movement_event event){movement_event_received(player_index, event);});
    fire_selected(input.fire);
    return true;
  });
}

bool mission_session::advance_frame(player_input &player_controls, frame_ratio const time_ratio, std::function<bool()> const &prepare_player) {
  frame_contacts.clear();
  frame_notices.clear();
  for(auto &member : crew) member.actor->begin_frame();
  for(size_t i{0};i!=crew.size();++i)crew[i].waiting=activity->waiting(i);
  for(size_t i{0};i!=flyers.size();++i)flyers[i].waiting=activity->waiting(flyer_id(i));
  for(size_t i{0};i!=bases.size();++i)bases[i].waiting=activity->waiting(base_id(i));
  // The expansion moved ground actors after effects/projectiles (0x5fb6e).
  // Base 0x5975e updates them first. This order also determines RNG use and
  // whether a shot sees the preceding or current turret/landing-craft state.
  auto const advance_bases{[&]{for(size_t index{0}; index != bases.size(); ++index) advance_base(index, time_ratio);}};
  // Alt+S gates the world before input; the same command can change the later power gate.
  if(!world_frozen){
    if(weapon_definitions.edition != game_edition::expansion) advance_bases();
    effects.advance(time_ratio);
    effects.advance_deployments(time_ratio,[&](vec3<int32_t> const &position){
      impact_request request;request.transform.translation=position;request.damage.armour=10000;request.area_radius=3000;
      return impact(projectile_owner{.actor{activity->size()},.target{}},request).has_value();
    },[&](size_t group,vec3<int32_t> const &position){
      auto &entry{activity->group(group)};if(!entry.members.empty())place_deployed(entry.members.front(),position);entry.activation_trigger.reset();
    },player().movement().position);
    player_weapons->set_manual_guidance(false);
    projectiles.advance(time_ratio, [&](projectile_owner &owner){
      projectile_guidance input;
      input.player_owner = owner.actor == player_index;
      input.damage_falloff=falloff_preference||recorded_playback;
      auto const attacker_side{activity->group(actor_group(owner.actor)).side};
      input.area_damage_multiplier=attacker_side==0?std::array<int16_t,4>{2800,2000,1400,1000}.at(static_cast<size_t>(settings.difficulty)):1024;
      input.manual_held = player_controls.fire;
      input.manual_axes = player_controls.camera_input();
      if(owner.actor < crew.size()) input.interference = crew[owner.actor].sensors.interfered;
      input.owner_tracks_target = activity->targets()[owner.actor].selected == owner.target;
      if(owner.target) {
        input.target_position = target_position(*owner.target, owner.target_component);
        input.target_detectable = observe().at(*owner.target).detectable;
        input.target_origin.emplace(observe().at(*owner.target).position);
      }
      return input;
    }, [&](projectile_owner const &owner, impact_request const &request){ return impact(owner, request); },
    [&](projectile_owner const &owner, projectile_guidance const &feedback){
      player_controls.fire = feedback.manual_held;
      player_controls.camera_input(feedback.manual_axes);
      if(feedback.manual_input_used) player_weapons->set_manual_guidance(true);
      if(feedback.owner_guiding && owner.actor < crew.size()) crew[owner.actor].fire.skip_selection = true;
    });
    if(weapon_definitions.edition == game_edition::expansion) advance_bases();
    activity->dispatch_groups([&](size_t const actor){
      if(actor < crew.size()) dispatch_herc(actor, time_ratio);
      else if(actor < base_id(0)) dispatch_flyer(actor - flyer_id(0), time_ratio);
    },[&](size_t group){return activate_group(group);});
  }
  if(!prepare_player())return false; // Original Q leaves after player controls, before perception and power.
  for(size_t i{0};i!=crew.size();++i)crew[i].waiting=activity->waiting(i);
  for(size_t i{0};i!=flyers.size();++i)flyers[i].waiting=activity->waiting(flyer_id(i));
  for(size_t i{0};i!=bases.size();++i)bases[i].waiting=activity->waiting(base_id(i));
  auto const observations{observe()};
  activity->scan_areas(observations);
  if(!world_frozen){
    activity->perceive(observations,time_ratio,random,[&](size_t first,size_t second){return !intersect_scenery_segment(terrain.colliders(),observations[first].position,observations[second].position);},[&](size_t observer,size_t target){notice_contact(observer,target,time_ratio);});
    for(size_t i{0}; i != crew.size(); ++i) if(!crew[i].waiting && !crew[i].actor->damage().consequences.dead) {
      crew[i].actor->update_power(time_ratio, settings.free_weapons(), i == player_index ? player_weapons->selection() : int16_t{-1}, [&](weapon_shot const &shot){launch(i, shot);});
      update_herc_sensors(i, time_ratio);
    }
  }
  if(auto const result{outcome_clock.advance(time_ratio, [&]{return mission_state();})}) pending_mission_prompt=result;
  // Phase-five reclamation follows firing: expired objects occupy their slots
  // through this frame's simulation, as in the original shared pools.
  effects.collect();
  projectiles.collect();
  return true;
}

bool mission_session::select_weapon(size_t const slot) { return player_weapons->select(slot); }
void mission_session::cycle_weapon() { player_weapons->cycle(); }
void mission_session::set_auto_tracking(bool enabled) { player_weapons->set_tracking(enabled); }
void mission_session::toggle_weapon_pair() { player_weapons->toggle_pair(); }
std::optional<size_t> mission_session::weapon_selection() const {
  auto const selected{player_weapons->selection()};
  return selected < 0 ? std::nullopt : std::optional<size_t>{static_cast<size_t>(selected)};
}
player_armament const &mission_session::player_weapon_controls() const { return *player_weapons; }
herc_sensor_state const &mission_session::player_sensors() const { return crew[player_index].sensors; }
std::optional<size_t> mission_session::target_selection() const { return displayed_target; }
std::optional<hud_target_state> mission_session::target_marker() const {
  if(!displayed_target) return {};
  auto const index{*displayed_target};
  int radius;
  if(index < crew.size()) radius = crew[index].definition->models.roots.front().header.radius;
  else if(index < base_id(0)) radius = flyers[index - flyer_id(0)].actor->model().roots.front().header.radius;
  else radius = bases[index - base_id(0)].actor->model().roots.front().header.radius;
  return hud_target_state{vec3<int32_t>{observe().at(index).position}, radius, activity->group(actor_group(index)).side, true, player_sensors().hud_locked};
}
std::optional<hud_waypoint_state> mission_session::next_waypoint() const {
  auto const &group{activity->group(crew[player_index].group)};
  if(group.route < 0) return {};
  auto const &route{routes.at(static_cast<size_t>(group.route))};
  int const next{group.route_cursor + 1};
  if(next < 0 || static_cast<size_t>(next) >= route.points.size()) return {};
  return hud_waypoint_state{vec3<int32_t>{points.at(static_cast<size_t>(route.points[static_cast<size_t>(next)]))}, static_cast<int16_t>(next)};
}
void mission_session::set_displayed_target(std::optional<size_t> const target) {
  if(target && *target >= activity->size()) throw std::out_of_range{"cockpit target"};
  displayed_target = instrument_target = target;
}
void mission_session::clear_player_input() { input_router.clear_device_state(); }
bool mission_session::player_command(uint16_t const code) {
  if(development_command(code))return true;
  if(player_weapons->command(code)) return true;
  if(code >= 2 && code <= 10) return player_weapons->click_control(code - 2, false); // 1..9
  if(code == 76) { // Keypad 5: 110a1 resets throttle and sets its ownership flag.
    player_input_actions actions{}; actions.stop = true;
    crew[player_index].actor->apply_player_controls(actions, frame_ratio_one);
  }
  else if(code == 14) input_router.request_centre();                             // Backspace
  else if(code == 15) cycle_target();                                       // Tab; view-specific gating belongs to the cockpit
  else if(code == 19) { auto &actor{*crew[player_index].actor}; actor.impacts().detectable = !actor.impacts().detectable; emit_sound(sample_sound_request{sound_action::play,static_cast<uint16_t>(actor.impacts().detectable ? 26 : 27)}); }
  else return false;
  return true;
}
bool mission_session::development_command(uint16_t code,std::optional<size_t> subject){
  if(code==0x402){if(weapon_definitions.edition!=game_edition::expansion){development.toggle_direction();crew[player_index].actor->movement_direction(development.movement_direction);}return true;}
  if(code==0x21f){if(recorded_playback)world_frozen=!world_frozen;return true;}
  if(code==0x619){
    if(weapon_definitions.edition==game_edition::expansion){
      auto const &body{player()};
      auto const destination{development_pod_destination(body.movement().position,body.movement().angles.z)};
      effects.add_deployment(std::nullopt,destination,random);return true;
    }
    auto const [actors,groups]{objective_facts()};
    // Original f744 considers hostile HERCs, not flyers or gun towers.
    bool const safe{objective_world{std::span{actors}.first(crew.size()),groups,player_index,mission_bounds::from_points(points)}.safe(player_index)};
    auto const observations{observe()};std::vector<development_contact> contacts;
    for(size_t i{};i<observations.size();++i){
      auto const &actor{observations[i]};
      bool const armed{i<base_id(0)||bases[i-base_id(0)].actor->profile().can_retaliate};
      contacts.push_back({vec3<int32_t>{actor.position},i<crew.size()?45000:i<base_id(0)?50000:35000,
        i<crew.size()?crew[i].definition->profile.base_combat_rating:int16_t{500},
        activity->group(actor_group(i)).side,armed&&!activity->waiting(i)&&!actor.dead&&!actor.disabled&&!actor.unavailable});
    }
    int const passes{development_damage_passes(contacts,player().movement().position,crew[player_index].side,safe)};
    for(int i{};i<passes;++i)crew[player_index].actor->random_damage(64,64,random,damage_events(player_index));
    return true;
  }
  auto &actor{*crew.at(subject.value_or(player_index)).actor};
  auto position{actor.body().movement().position};auto angles{actor.body().movement().angles};
  if(!development.reposition(code,position,angles))return false;
  actor.place(position,angles);return true;
}
random_state &mission_session::interface_random() { return visual_random; }
music_contacts mission_session::music_observations()const{
  music_contacts result;auto const observations{observe()};auto const &knowledge{activity->actor(player_index).awareness};auto const &body{player()};
  cockpit_observer const observer{vec3<int32_t>{body.movement().position},body.movement().angles.z,body.torso().angle,crew[player_index].side,settings.detection_range()};
  for(size_t i{};i<observations.size();++i){auto const &a{observations[i]};auto const &group{activity->group(actor_group(i))};
    vec3<int32_t> delta;for(unsigned int axis{};axis<3;++axis)delta[axis]=wrap_long(static_cast<int64_t>(a.position[axis])-observer.position[axis]);
    cockpit_target const target{vec3<int32_t>{a.position},group.side,activity->actor(i).awareness.seen,knowledge.known[i]!=0,a.dead,a.disabled};
    music_contact contact{fast_distance(delta),group.side,a.disabled,a.dead,activity->waiting(i),can_target_from_cockpit(observer,target)};
    result.actors.push_back(contact);if(i>=flyer_id(0)&&i<base_id(0))result.flyers.push_back(contact);
  }return result;
}
void mission_session::cycle_target() {
  auto const observations{observe()};
  std::vector<cockpit_target> candidates;
  auto const &knowledge{activity->actor(player_index).awareness};
  for(size_t i{0}; i != observations.size(); ++i) {
    auto const &actor{observations[i]};
    candidates.push_back({vec3<int32_t>{actor.position}, activity->group(actor_group(i)).side, activity->actor(i).awareness.seen, knowledge.known[i] != 0, actor.dead, actor.disabled});
  }
  auto const &body{player()};
  cockpit_observer const observer{vec3<int32_t>{body.movement().position}, body.movement().angles.z, body.torso().angle, crew[player_index].side, settings.detection_range()};
  set_displayed_target(cycle_cockpit_target(candidates, observer, displayed_target));
}
std::optional<player_input_actions> mission_session::step_player(player_input input, frame_ratio const ratio, bool const external_control, rectangle const aiming_viewport, std::function<bool(player_input &)> const &dispatch_command, std::function<void(player_input_actions const &)> const &camera_controls) {
  player_input_actions actions;
  bool const completed{advance_frame(input, ratio, [&]{
    bool const continue_frame{!dispatch_command || dispatch_command(input)};
    if(input.command) player_command(input.command);
    auto &actor{*crew[player_index].actor};
    player_input_context context{.external_control{external_control}, .disabled{actor.damage().consequences.disabled}, .auto_tracking{player_weapons->auto_tracking()}, .position{vec3<int32_t>{actor.body().movement().position}}, .target{}};
    context.release_action=input.release_action;
    context.viewport_min = {aiming_viewport.left, aiming_viewport.top};
    context.viewport_max = {aiming_viewport.right, aiming_viewport.bottom};
    if(auto const selected{activity->targets()[player_index].selected}) {
      auto const observations{observe()};
      vec3<int16_t> offset;
      if(*selected < crew.size()) offset = crew[*selected].actor->profile().camera_focus;
      else if(*selected >= base_id(0)) offset.z = bases.at(*selected-base_id(0)).actor->profile().target_height;
      context.target = player_input_target{vec3<int32_t>{observations[*selected].position}, vec3<int16_t>{offset}, observations[*selected].dead};
    }
    actions = input_router.route(input, context);
    if(camera_controls)camera_controls(actions);
    actor.apply_player_controls(actions, ratio, [&](movement_event event){movement_event_received(player_index, event);});
    if(actions.fire_allowed) fire_selected(actions.fire);
    if(actions.cycle_target) cycle_target();
    if(input.cycle_weapon)cycle_weapon();
    if(input.toggle_radar)player_command(19);
    return continue_frame;
  })};
  if(!completed)return {};
  return actions;
}
void mission_session::change_player_shield_balance(bool const toward_front) {
  change_shield_balance(crew[player_index].actor->power().shields, toward_front);
}
int16_t mission_session::exchange_player_instruments(int16_t const requested_throttle, std::optional<bool> const shield_to_front) {
  auto &actor{*crew[player_index].actor};
  if(shield_to_front) change_shield_balance(actor.power().shields, *shield_to_front);
  return actor.exchange_throttle(requested_throttle);
}
void mission_session::initialise_weapon_instruments() { player_weapons->initialise_displays(); exchange_weapon_instruments(0); }
void mission_session::exchange_weapon_instruments(frame_ratio const ratio) {
  player_weapons->exchange_displays(player_weapon_range, crew[player_index].sensors.locks, ratio);
}
void mission_session::weapon_group_control(unsigned int const button) { player_weapons->request_group_control(button); }
void mission_session::cycle_weapon_group(bool const reverse) { player_weapons->cycle_requested_group(reverse); }
void mission_session::weapon_control(unsigned int const control_number, bool const secondary) {
  if(player_weapons->click_control(control_number, secondary)&&!secondary)emit_sound(sample_sound_request{sound_action::play,26});
}
void mission_session::weapon_charge_request(unsigned int const control_number, int32_t const value) { player_weapons->request_charge(control_number, value); }
void mission_session::update_player_interface() {
  auto &member{crew[player_index]};
  auto &relationship{activity->targets()[player_index]};
  int32_t distance{0};
  if(relationship.selected) {
    auto const target{combat_position(*relationship.selected).position};
    auto const &position{member.actor->body().movement().position};
    vec3<int32_t> delta;
    for(unsigned int axis{0}; axis != 3; ++axis) delta[axis] = wrap_long(static_cast<int64_t>(target[axis]) - position[axis]);
    distance = fast_distance(delta);
  }
  player_weapon_range = distance;
  player_weapons->finish_manual_guidance();
  player_weapons->update_selection(distance, member.sensors.locks);
  player_weapons->exchange_group_controls();
  player_weapons->refresh_pairs();
  // The original HUD exchanges target ownership after weapons and power. Enter
  // changes the displayed target first; guidance sees it on the following frame.
  if(relationship.selected != instrument_target) {
    member.sensors.hud_locked = false;
    assign_target(activity->targets(), player_index, instrument_target);
    if(!instrument_target && player_weapons->auto_tracking()) input_router.request_centre();
  }
}
void mission_session::refresh_target_instruments(bool visible,bool validate){
  // 2cf28 paints the HUD before validating the shell's selected target.
  // 155a8 exchanges that cached HUD value; 59c08 then paints the HUD again.
  if(visible)instrument_target=displayed_target;
  if(!validate||!displayed_target)return;
  auto index{*displayed_target};auto observations{observe()};
  auto const &body{player()};auto const &target{observations.at(index)};
  cockpit_observer observer{vec3<int32_t>{body.movement().position},body.movement().angles.z,body.torso().angle,crew[player_index].side,settings.detection_range()};
  cockpit_target contact{vec3<int32_t>{target.position},activity->group(actor_group(index)).side,activity->actor(index).awareness.seen,activity->actor(player_index).awareness.known[index]!=0,target.dead,target.disabled};
  if(!detectable_from_cockpit(observer,contact))displayed_target.reset();
}
herc_actor const &mission_session::player_actor() const { return *crew.at(player_index).actor; }
world_effects &mission_session::visual_effects() { return effects; }
std::vector<ground_flash> mission_session::ground_models() {
  std::vector<ground_flash> result;
  for(auto &member:crew)for(auto &attachment:member.actor->ground_attachments())
    if(attachment.active)result.push_back(effects.ground_attachment(
      static_cast<size_t>(attachment.shape),attachment.position,attachment.heading));
  for(auto const &flash:effects.ground_flashes())result.push_back(flash);
  return result;
}
world_projectiles const &mission_session::ordnance() const { return projectiles; }
std::span<combat_notice const> mission_session::combat_notices() const { return frame_notices; }

void mission_session::fire_selected(bool const pressed) {
  auto &actor{*crew[player_index].actor};
  bool fired{false};
  player_weapons->fire(pressed, [&](size_t const slot){
    fired = true;
    for(auto const &shot : actor.discharge_aimed_slot(slot, settings.free_weapons())) launch(player_index, shot);
  });
  if(weapon_definitions.edition == game_edition::expansion) {
    player_fire_line.clear();
    if(fired) {
      auto const &body{actor.body()};auto const &motion{body.movement()};
      int count{20};
      if(auto const target{activity->targets()[player_index].selected}) {
        auto const point{combat_position(*target).position};
        count = std::clamp(fast_distance({wrap_long(static_cast<int64_t>(point.x)-motion.position.x),wrap_long(static_cast<int64_t>(point.y)-motion.position.y),wrap_long(static_cast<int64_t>(point.z)-motion.position.z)}) >> 12, 1, 40);
      }
      auto const step{rotate_vector({0,4096,0},rotation_transform({0,0,static_cast<uint16_t>(motion.angles.z-body.torso().angle)}))};
      vec2<int32_t> point{motion.position.x,motion.position.y};
      for(int i{};i<count;++i){point.x=wrap_long(static_cast<int64_t>(point.x)+step.x);point.y=wrap_long(static_cast<int64_t>(point.y)+step.y);player_fire_line.push_back(point);}
    }
  }
}

herc_damage_events mission_session::damage_events(size_t const index) {
  herc_damage_events events;
  events.explosion = [&, index](int16_t const definition, vec3<int32_t> const &position, random_state &rng){ effects.add_explosion(definition, position, rng); };
  events.debris = [&, index](int16_t const definition, vec3<int32_t> const &position, random_state &rng){ effects.add_debris(definition, position, rng); };
  events.attached_smoke = [&, index](int16_t const part, vec3<int16_t> const &position){
    frame_notices.push_back({combat_notice_kind::attached_smoke, index, part, 0, vec3<int32_t>{position}});
    effects.add_smoke(0, [this, index, part, offset = vec3<int16_t>{position}]{
      auto const &member{crew.at(index)}; auto const &body{member.actor->body()};
      auto const *node{part < 0 ? nullptr : find_model_part(member.definition->models.roots.front(), part)};
      auto const *transform{node ? &body.model().transforms()[static_cast<size_t>(node->header.transform)] : nullptr};
      return smoke_attachment_position(body.placement(), std::optional<vec3<int16_t>>{offset}, transform);
    }, visual_random);
  };
  events.weapon_ejected = [&, index](size_t const slot){
    frame_notices.push_back({combat_notice_kind::ejected_weapon, index, static_cast<int16_t>(slot), 0, {}});
    auto const &member{crew.at(index)}; auto const &body{member.actor->body()};
    auto const &mount{member.definition->mounts.at(slot)};
    auto const &definition{weapon_definitions.weapons.at(static_cast<size_t>(member.actor->weapons().slots()[slot]->definition))};
    auto const *node{find_model_part(member.definition->models.roots.front(), mount.model_part)};
    auto const hardpoint{compose_transforms(body.model().transforms()[static_cast<size_t>(node->header.transform)], body.placement())};
    auto const centre{smoke_attachment_position(body.placement(), {}, &body.model().transforms()[static_cast<size_t>(find_model_part(member.definition->models.roots.front(), member.definition->profile.camera_part)->header.transform)])};
    effects.add_detached_weapon(create_detached_weapon(definition, mount, hardpoint, centre));
  };
  events.salvage = [&, index](int16_t const weapon, int16_t const quality){ frame_notices.push_back({combat_notice_kind::salvage, index, weapon, quality, {}}); recovered_weapons.emplace_back(weapon,quality); };
  events.shield_warning = [&, index]{ frame_notices.push_back({combat_notice_kind::shield_warning, index, 0, 0, {}});announce_system(8); };
  events.cockpit_damage = [&, index](size_t const part){ if(cockpit_impact_handler)cockpit_impact_handler(); frame_notices.push_back({combat_notice_kind::cockpit_damage, index, static_cast<int16_t>(part), 0, {}}); };
  return events;
}

ground_damage_events mission_session::base_damage_events(size_t const index) {
  ground_damage_events events;
  events.explosion = [&](int16_t const definition, vec3<int32_t> const &position, random_state &rng){ effects.add_explosion(definition, position, rng); };
  events.debris = [&](int16_t const definition, vec3<int32_t> const &position, random_state &rng){ effects.add_debris(definition, position, rng); };
  events.attached_smoke = [&, index](vec3<int16_t> const &position){
    frame_notices.push_back({combat_notice_kind::attached_smoke, base_id(index), -1, 1, vec3<int32_t>{position}});
    effects.add_smoke(1, [this, index, offset = vec3<int16_t>{position}]{
      return smoke_attachment_position(bases.at(index).actor->placement(), std::optional<vec3<int16_t>>{offset});
    }, visual_random);
  };
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

bool mission_session::blast(projectile_owner const &owner, vec3<int32_t> const &centre, int16_t radius, int16_t amount) {
  auto distance=[&](vec3<int32_t> const &p){vec3<int32_t> d;for(unsigned i{};i<3;++i)d[i]=wrap_long(static_cast<int64_t>(p[i])-centre[i]);return fast_distance(d);};
  bool contacted{};
  auto defeated=[&](size_t id,bool before,bool after){if(!before&&after&&owner.actor<activity->size()){if(owner.actor==player_index&&owner.target==id)announce_system(3);credit_defeat(owner.actor,id);}};
  for(size_t i{};i<crew.size();++i){auto &m{crew[i]};if(m.waiting||distance(m.actor->body().movement().position)-m.definition->profile.collision_radius>=radius)continue;
    contacted=true;bool before{m.actor->damage().consequences.dead||m.actor->damage().consequences.disabled};
    m.actor->blast_damage(centre,radius,amount,random,damage_events(i),i==player_index&&settings.player_is_invulnerable(weapon_definitions.edition));
    defeated(i,before,m.actor->damage().consequences.dead||m.actor->damage().consequences.disabled);
  }
  for(size_t i{};i<flyers.size();++i){auto &m{flyers[i]};auto d{distance(m.actor->motion().position)};if(m.waiting||d>=radius)continue;
    contacted=true;bool before{m.actor->dead()};m.actor->direct_damage(wrap_word(wrap_long(static_cast<int64_t>(radius-d)*amount)/radius),random,flyer_events(i),owner.actor==player_index&&owner.target==flyer_id(i));defeated(flyer_id(i),before,m.actor->dead());
  }
  for(size_t i{};i<bases.size();++i){auto &m{bases[i]};auto const &profile{m.actor->profile()};auto d{distance(m.actor->placement().translation)-(profile.collision_enabled?profile.contact_radius:0)};
    if(m.waiting||d>=radius)continue;
    contacted=true;if(!profile.collision_enabled||m.actor->is_wreck())continue;
    bool before{m.actor->dead()};m.actor->direct_damage(wrap_word(wrap_long(static_cast<int64_t>(radius-d)*amount)/radius),random,base_damage_events(i));defeated(base_id(i),before,m.actor->dead());
  }
  return contacted;
}

std::optional<int32_t> mission_session::impact(projectile_owner const &owner, impact_request const &incoming_request) {
  auto request{incoming_request};
  if(request.area_radius) return blast(owner, request.transform.translation, request.area_radius, request.damage.armour) ? std::optional<int32_t>{0} : std::nullopt;
  // Expansion 0x2181c scales a friendly shot once before actor traversal.
  // Its actor hit routines consume that result directly; base routines scale locally.
  if(weapon_definitions.edition == game_edition::expansion) {
    impact_conditions const conditions{.attacker_side{activity->group(actor_group(owner.actor)).side}, .difficulty{settings.difficulty}};
    request.damage.armour = adjust_impact_damage(request.damage.armour, conditions);
    request.damage.shield = adjust_impact_damage(request.damage.shield, conditions);
  }
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
        impact_conditions const conditions{.player{index == player_index}, .attacker_side{actors.at(owner.actor).side}, .difficulty{settings.difficulty}, .difficulty_override{settings.campaign_mode != 0}, .side{actors[index].side}, .invulnerable{settings.player_is_invulnerable(weapon_definitions.edition)}};
        bool const eligible{!actors[index].dead&&!actors[index].disabled};
        auto const result{crew[index].actor->receive_impact(incoming, conditions, random, events)};
        auto const &state{crew[index].actor->damage().consequences};
        if(owner.actor==player_index&&activity->targets()[player_index].selected==index){
          if(!actors[index].disabled&&state.disabled)announce_system(7);
          if(!actors[index].dead&&state.dead)announce_system(3);
        }
        if(eligible&&(state.dead||state.disabled))credit_defeat(owner.actor,index);
        else if(!actors[index].dead&&state.dead&&crew[index].group==crew[player_index].group&&index!=player_index)announce(index,37,true);
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
        if(!actors[flyer_id(index)].dead&&flyers[index].actor->dead()){
          if(owner.actor==player_index&&activity->targets()[player_index].selected==flyer_id(index))announce_system(3);
          credit_defeat(owner.actor,flyer_id(index));
        }
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
      .waiting{member.waiting}, .dead{member.actor->dead()}, .disabled{false},
      .inactive{member.actor->profile().edition == game_edition::expansion && !member.actor->is_turret() && !member.actor->is_landing_craft()}, .quiet{activity->actor(base_id(index)).awareness.event_busy}, .hit_enemy{false}, .trigger{activity->actor(base_id(index)).contact_trigger},
      .hit{[&, index](impact_request const &incoming) -> std::optional<int32_t> {
        impact_conditions const conditions{.attacker_side{actors.at(owner.actor).side}, .difficulty{settings.difficulty}, .difficulty_override{settings.campaign_mode != 0}, .side{bases[index].side}};
        auto const result{bases[index].actor->receive_impact(incoming, conditions, random, base_damage_events(index))};
        if(!actors[base_id(index)].dead&&bases[index].actor->dead()){
          if(owner.actor==player_index&&activity->targets()[player_index].selected==base_id(index))announce_system(3);
          credit_defeat(owner.actor,base_id(index));
        }
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
  if(actors[owner.actor].hit_enemy) activity->actor(owner.actor).awareness.alerted = true;
  return result;
}

std::vector<scene_object> mission_session::visible_objects(bool include_player) {
  auto models{herc_models(false, include_player)};
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

std::vector<scene_model> mission_session::herc_models(bool const roster_only, bool const include_player) {
  std::vector<scene_model> result;
  result.reserve(crew.size());
  for(auto &member : crew) {
    if(roster_only ? !member.player_roster : member.waiting) continue;
    if(!include_player && &member == &crew[player_index]) continue;
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

camera_target mission_session::view_target(std::optional<size_t> actor) const {
  auto const &member{crew.at(actor.value_or(player_index))};
  camera_target result{.angles{vec3<uint16_t>{member.actor->body().movement().angles}}, .placement{member.actor->body().placement()}, .attachment{std::nullopt}};
  auto const part{find_model_part(member.definition->models.roots.front(), member.definition->profile.camera_part)};
  if(part) {
    if(part->header.transform < 0) throw std::runtime_error{"HERC camera part has no skeletal transform"};
    result.attachment = member.actor->body().model().transforms()[static_cast<size_t>(part->header.transform)];
  }
  return result;
}

camera_state mission_session::make_camera(camera_mode const mode, std::optional<size_t> actor) const {
  auto const &profile{crew.at(actor.value_or(player_index)).definition->profile};
  camera_state result;
  result.mode = mode;
  result.focus_offset = profile.camera_focus;
  result.attachment_offset = profile.camera_offset;
  update_camera(result, view_target(actor));
  return result;
}

std::vector<sound_request> mission_session::take_sounds() { return std::exchange(sound_requests,{}); }

} // namespace earthsiege
