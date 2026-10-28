#include "mission_session.h"
#include <algorithm>
#include <numeric>
#include <stdexcept>
#include "simulation/fixed_math.h"
#include "simulation/herc_assessment.h"
#include "simulation/herc_guard.h"
#include "simulation/herc_strategy.h"
#include "simulation/model_math.h"

namespace earthsiege {
namespace {
bool incapacitated(engagement_state const &state) { return state.dead || state.disabled || state.unavailable; }
int32_t distance(vec3<int32_t> const &a, vec3<int32_t> const &b) {
  return fast_distance({wrap_long(static_cast<int64_t>(a.x) - b.x), wrap_long(static_cast<int64_t>(a.y) - b.y), wrap_long(static_cast<int64_t>(a.z) - b.z)});
}

}

herc_combat_position mission_session::combat_position(size_t const index) const {
  if(index < crew.size()) {
    auto const &body{crew[index].actor->body()};
    return {vec3<int32_t>{body.movement().position}, body.movement().angles.z, body.torso().angle};
  }
  if(index < base_id(0)) {auto const &actor{*flyers[index-flyer_id(0)].actor}; return {vec3<int32_t>{actor.motion().position}, actor.angles().z, 0};}
  auto const &actor{*bases.at(index-base_id(0)).actor};
  return {vec3<int32_t>{actor.placement().translation}, transform_angles(actor.placement()).z, 0};
}

std::vector<contact_actor> mission_session::contact_bodies() const {
  std::vector<contact_actor> result;
  for(auto const &member : crew) { auto body{member.actor->body().contact_geometry()}; body.waiting = member.waiting; result.push_back(body); }
  // Zero-radius flyers retain their actor ordinals in the original registry.
  for(auto const &member : flyers) result.push_back({.position{vec3<int32_t>{member.actor->motion().position}}, .herc{false}, .waiting{member.waiting}});
  for(auto const &member : bases) result.push_back({.position{vec3<int32_t>{member.actor->placement().translation}}, .radius{member.actor->profile().contact_radius}, .herc{false}, .waiting{member.waiting}});
  return result;
}

void mission_session::enter_herc(size_t const index, behavior const type) {
  auto &member{crew.at(index)};
  member.attack = {}; member.travel = {}; member.flee = {}; member.skirt = {}; member.ram = {};
  enter_behavior(activity->decisions()[index].controller, type, activity->entry_stagger());
}
void mission_session::resume_herc_orders(size_t const index) {
  assign_target(activity->targets(), index, std::nullopt);
  enter_herc(index, initial_herc_behavior(index == player_index, activity->group(crew[index].group).order, crew[index].definition->profile.torso.maximum));
}
void mission_session::movement_event_received(size_t const index, movement_event const event) {
  if(event == movement_event::collapsed) crew[index].actor->random_damage(120, 150, random, damage_events(index));
}
void mission_session::move_herc(size_t const index, movement_request const &request, frame_ratio const ratio) {
  auto &member{crew.at(index)}; auto &actor{*member.actor};
  actor.prepare_requested(request.speed, request.turn, ratio, [&](movement_event event){movement_event_received(index, event);}, [&](int16_t &speed, int16_t &turn){
    if(index == player_index) return;
    auto const &motion{actor.body().movement()};
    auto bodies{contact_bodies()}; bodies.at(index).radius = 0;
    movement_request adjusted{request.flags, speed, turn};
    avoid_obstacles(adjusted, {vec3<int32_t>{motion.position}, motion.angles.z, motion.gait_rate, motion.collision_recovery, member.collision_bias, motion.reverse_recovery}, bodies,
      [&](avoidance_probe kind, auto const &start, auto const &end){return obstacle_distance(kind, start, end);});
    speed = adjusted.speed; turn = adjusted.turn;
  });
}

void mission_session::navigate_herc(size_t const index, frame_ratio const ratio) {
  auto &member{crew.at(index)}; auto &actor{*member.actor}; auto &group{activity->group(member.group)};
  auto const &motion{actor.body().movement()}; auto const command{activity->decisions()[index].command};
  movement_request request;
  if(command != 0) request = {.speed{member.requested_speed ? member.requested_speed : int16_t{170}}, .turn{static_cast<int16_t>(wrap_word(bearing_angle(motion.position, member.command_position) - motion.angles.z) >> 4)}};
  else if(group.members.front() == index) {
    if(group.route >= 0) request = follow_route(motion.position, motion.angles.z, member.requested_speed, routes.at(static_cast<size_t>(group.route)), points, group.route_cursor);
  } else {
    auto const &leader{crew.at(group.members.front()).actor->body().movement()};
    auto const &offset{layout.formation_offset(actor_kind::herc, static_cast<size_t>(group.formation), activity->actor(index).member - 1)};
    request = follow_formation(motion.position, motion.angles.z, leader.position, leader.angles.z, leader.gait_rate, offset);
  }
  move_herc(index, request, ratio);
  actor.impacts().detectable = activity->actor(group.members.front()).player ? member.radar_active : activity->actor(index).persistent_signature;
  actor.centre_aim(0, ratio);
}

bool mission_session::contact_herc(size_t const index) {
  auto const bodies{contact_bodies()};
  if(auto const hit{find_actor_contact(bodies, index)}) {
    if(hit->other < crew.size()) crew[hit->other].contact_received = true;
    frame_contacts.push_back(*hit);
    if(hit->damage > 0 && hit->other < crew.size()) {
      for(auto const victim : {index, hit->other}) crew[victim].actor->radial_damage(hit->position, ramming_damage_radius, wrap_word(hit->damage), random, damage_events(victim), victim == player_index && settings.player_is_invulnerable());
    }
    return true;
  }
  auto const &position{crew[index].actor->body().movement().position};
  std::vector<scenery_collider> buildings;
  for(auto const &member : bases) if(auto collider{member.actor->occupancy()}) buildings.push_back(*collider);
  if(scenery_contact_height(buildings, position) != 0) return true;
  return scenery_contact_height(terrain.colliders(), position) != 0;
}

void mission_session::advance_herc(size_t const index, frame_ratio const ratio, bool const ramming) {
  auto &member{crew.at(index)}; auto &actor{*member.actor};
  if(ramming) {
    actor.advance(ratio);
    if(contact_herc(index) || member.contact_received) {
      auto const centre{actor.body().movement().position};
      for(size_t i{0}; i != crew.size(); ++i) if(!crew[i].waiting && distance(centre, crew[i].actor->body().movement().position) < 3000)
        crew[i].actor->radial_damage(centre, 3000, 2000, random, damage_events(i), i == player_index && settings.player_is_invulnerable());
      if(index != player_index || !settings.player_is_invulnerable()) for(size_t section{0}; section != actor.damage().present.size(); ++section)
        if(actor.damage().present[section]) actor.direct_damage(section, 32000, random, damage_events(index));
    }
    return;
  }
  actor.advance(ratio, [&](herc_body const &){return contact_herc(index);}, [&]{
    if(index != player_index) {
      member.collision_bias = static_cast<int16_t>((random.next() & 1) ? -5000 : 5000);
      actor.recover_from_collision(20000, actor.body().movement().gait_rate > 0);
    }
  });
}

void mission_session::damage_consequences_changed(size_t const index, damage_consequences const &previous, damage_consequences const &current) {
  auto &state{activity->decisions()[index]}; state.dead = current.dead; state.disabled = current.disabled;
  if(!previous.disabled && current.disabled && !previous.dead) {
    activity->notify_destroyed(index, observe());
    if(auto const trigger{activity->actor(index).hit_trigger}) activity->activate(*trigger);
    enter_herc(index, behavior::disabled);
  }
  if(!previous.dead && current.dead) {
    if(!current.disabled) {activity->notify_destroyed(index, observe()); if(auto const trigger{activity->actor(index).hit_trigger}) activity->activate(*trigger);}
    if(crew[index].actor->profile().remove_when_destroyed) {
      auto const &motion{crew[index].actor->body().movement()};
      auto position{motion.position}; position.z = -100000;
      crew[index].actor->place(position, motion.angles);
      enter_herc(index, behavior::in_limbo);
    } else enter_herc(index, behavior::herc_dead);
  }
}

vec3<int32_t> mission_session::guard_anchor(size_t const index) const {
  auto const &member{crew.at(index)};
  switch(activity->decisions()[index].command) {
    case 6: return vec3<int32_t>{member.command_position};
    case 3: return combat_position(member.followed.value()).position;
    case 5: return combat_position(member.designated.value()).position;
    default: break;
  }
  auto const &group{activity->group(member.group)};
  if(group.representative) return combat_position(*group.representative).position;
  if(group.route < 0) throw std::logic_error{"guard has neither an objective nor a route"};
  return vec3<int32_t>{points.at(routes.at(static_cast<size_t>(group.route)).points.front())};
}

bool mission_session::herc_target_completed(size_t const index) const {
  auto const target{activity->targets()[index].selected.value()};
  auto const &own{activity->decisions()[index]};
  auto const observations{observe()};
  if(own.command == 4 && crew[index].designated) {
    auto const type{activity->decisions()[target].controller.type};
    auto const properties{type ? describe(*type).properties : uint16_t{0}};
    return (properties & 32 ? 2 : properties & 16 ? 1 : 0) > crew[index].stop_status;
  }
  auto const &objectives{activity->group(crew[index].group).objectives};
  if(std::ranges::find(objectives, target) != objectives.end()) return observations[target].dead;
  return observations[target].dead || observations[target].disabled || observations[target].unavailable;
}

bool mission_session::enter_skirting(size_t const index) {
  auto &member{crew[index]};
  if(!member.obstructed) return false;
  auto const previous{activity->decisions()[index].controller.type};
  auto const target{activity->targets()[index].selected.value()};
  enter_herc(index, behavior::skirting);
  member.skirt.previous = previous;
  member.skirt.distance = distance(combat_position(index).position, combat_position(target).position);
  return true;
}

bool mission_session::target_path_blocked(size_t const index) const {
  auto const target{activity->targets()[index].selected.value()};
  auto const start{combat_position(target).position}, end{combat_position(index).position};
  std::vector<scenery_collider> buildings;
  for(auto const &member : bases) if(member.side == crew[index].side)
    if(auto const shape{member.actor->obstruction()}) buildings.push_back(*shape);
  if(intersect_scenery_segment(buildings, start, end)) return true;
  return intersect_scenery_segment(terrain.colliders(), start, end).has_value();
}

void mission_session::reconsider_herc(size_t const index) {
  auto &member{crew[index]}; auto &actor{*member.actor};
  auto const &group{activity->group(member.group)};
  auto const observations{observe()};
  for(size_t i{0}; i != observations.size(); ++i) {
    activity->decisions()[i].dead = observations[i].dead;
    activity->decisions()[i].disabled = observations[i].disabled;
  }
  reconsider_herc_attack(activity->decisions(), activity->targets(), index, group.members,
    {group.order, activity->actor(group.members.front()).player, member.radar_active, actor.impacts().ecm_remaining}, actor.impacts().detectable, {
    .select{[&](target_selection_flags flags){return activity->select(observe(), index, flags, std::nullopt, random);}},
    .target_kind{[&](size_t target){return combat_kind(activity->actor(target).kind);}},
    // 0x174ca invokes each eligible follower's own strategy synchronously.
    .alert_follower{[&](size_t follower){reconsider_herc(follower);}},
    .resume_orders{[&]{resume_herc_orders(index);}},
    .retreat{[&]{
      auto const selected{activity->targets()[index].selected.value()};
      herc_retreat_inputs input{.incapacitated{incapacitated(activity->decisions()[index])}, .target_kind{combat_kind(activity->actor(selected).kind)},
        .target_can_retaliate{selected >= base_id(0) && bases[selected-base_id(0)].actor->profile().can_retaliate},
        .caution{actor.total_damage_fraction()}, .internal_damage{}, .attackers{activity->targets()[index].attackers}, .strength{actor.combat_rating()}};
      for(size_t i{0}; i != retreat_components.size(); ++i) input.internal_damage[i] = actor.internal_damage_fraction(retreat_components[i]);
      auto const result{assess_herc_retreat(input, member.aim_tolerance, [&]{
        int32_t sum{0};
        for(size_t i{0}; i != crew.size(); ++i) if(activity->targets()[i].selected == index) sum = wrap_long(static_cast<int64_t>(sum) + crew[i].actor->combat_rating());
        return sum;
      })};
      if(result == herc_retreat_action::resume_orders) resume_herc_orders(index);
      else if(result == herc_retreat_action::flee) enter_herc(index, behavior::fleeing);
      return result != herc_retreat_action::none;
    }},
    .compare_strength{[&]{auto const target{activity->targets()[index].selected.value()}; return compare_herc_strength(actor.combat_rating(), crew.at(target).actor->combat_rating(), combat_actor_kind::herc, random);}},
    .choose_component{[&]{return actor.choose_aimed_component(*crew.at(activity->targets()[index].selected.value()).actor, random);}},
    .enter{[&](behavior type){enter_herc(index, type);}},
  });
}

void mission_session::aim_herc(size_t const index, size_t const target, int16_t const facing, frame_ratio const ratio) {
  auto &member{crew[index]}; auto &actor{*member.actor};
  auto const own{combat_position(index)}, other{combat_position(target)};
  auto const kind{combat_kind(activity->actor(target).kind)};
  auto const selection{select_herc_aim_point(kind, own.position, own.heading, actor.profile().torso.maximum, other.position, activity->decisions()[index].aimed_component)};
  if(selection.mode == herc_aim_mode::centre_controls) { actor.centre_aim(0, ratio); return; }
  auto const point{target_position(target, selection.component)};
  aim_and_fire_herc(member.fire, actor.weapons(), weapon_definitions, own.position, {vec3<int32_t>{point}, other.heading, facing},
    {member.side != 0, settings.campaign_mode != 0, settings.difficulty}, random, {
    .select_weapon{[&](int32_t range, int16_t direction){
      auto &decision{activity->decisions()[index]};
      herc_weapon_selection_state state{member.sensors.locks, member.aim_tolerance, actor.impacts().ecm_remaining, actor.impacts().detectable, decision.unavailable};
      auto const slot{select_herc_weapon(actor.weapons(), weapon_definitions, state, kind, range, direction, random, mission_event_delay,
        [&](bool front){if(target >= crew.size()) return int16_t{0}; auto const &shield{crew[target].actor->power().shields}; return front ? shield.front : shield.rear;},
        [&]{if(auto const trigger{activity->actor(index).hit_trigger}) activity->activate(*trigger);})};
      actor.impacts().detectable = state.detectable; decision.unavailable = state.unavailable;
      return slot;
    }},
    .target_speed{[&]{return actor_speed(target);}},
    .aim{[&](vec3<int32_t> const &aim){return actor.aim_at(aim, ratio);}},
    .fire{[&](size_t slot){for(auto const &shot : actor.discharge_aimed_slot(slot, index == player_index && settings.free_weapons())) launch(index, shot);}},
  });
}

void mission_session::update_herc_sensors(size_t const index, frame_ratio const ratio) {
  auto &member{crew[index]}; auto &actor{*member.actor}; auto const &body{actor.body()};
  herc_sensor_inputs input{.position{vec3<int32_t>{body.movement().position}}, .heading{body.movement().angles.z}, .torso{body.torso().angle}, .player{index == player_index}, .selected_channel{}, .target{}};
  for(auto const &slot : actor.weapons().slots()) if(slot) {
    if(auto const ecm{std::get_if<ecm_weapon>(&slot->mechanism)}) {
      input.ecm_active = input.player ? ecm->enabled : actor.impacts().detectable;
      input.ecm_pulse = ecm->pulse;
    }
    if(auto const channel{weapon_lock_channel(*slot, weapon_definitions)})
      if(auto const supply{slot->ammunition()}) input.missile_counts[*channel] = wrap_word(input.missile_counts[*channel] + supply->rounds);
  }
  if(input.player && selected_weapon) input.selected_channel = weapon_lock_channel(actor.weapons().slots()[*selected_weapon].value(), weapon_definitions);
  auto &relationship{activity->targets()[index]};
  if(relationship.selected) {
    auto const target{*relationship.selected}; auto const observations{observe()};
    input.target = herc_sensor_target{vec3<int32_t>{observations[target].position}, target < crew.size(), observations[target].detectable, target < crew.size() && crew[target].sensors.ecm_active};
  }
  advance_herc_sensors(member.sensors, actor.impacts(), relationship.changed, input, ratio, random,
    [&]{activity->actor(relationship.selected.value()).awareness.event_busy = true;});
}

void mission_session::dispatch_herc(size_t const index, frame_ratio const ratio) {
  auto &member{crew.at(index)}; auto &actor{*member.actor};
  auto &state{activity->decisions()[index]}; auto &group{activity->group(member.group)};
  auto const follow{[&]{
    movement_request request;
    if(group.route >= 0) {auto const &motion{actor.body().movement()}; request = follow_route(motion.position, motion.angles.z, member.requested_speed, routes.at(static_cast<size_t>(group.route)), points, group.route_cursor);}
    move_herc(index, request, ratio);
  }};
  auto const attack{[&](behavior type){
    if(enter_skirting(index)) return false;
    auto const target{activity->targets()[index].selected.value()};
    return attack_with_herc(type, member.attack, state.command,
      {combat_position(index), combat_position(target), actor.impacts().damage_received, incapacitated(state), incapacitated(activity->decisions()[target]), herc_target_completed(index), target >= base_id(0) && bases[target-base_id(0)].actor->profile().can_retaliate}, ratio, {
      .move{[&](movement_request const &request){move_herc(index, request, ratio);}},
      .aim{[&](int16_t facing){aim_herc(index, target, facing, ratio);}},
      .completed{[&, target, type]{
        auto const observations{observe()};
        return incapacitated(state) || ((type == behavior::attacking_base || type == behavior::attacking_flyer) ? observations[target].dead || observations[target].disabled || observations[target].unavailable : herc_target_completed(index));
      }},
    });
  }};
  herc_order_callbacks const orders{
    .position{[&](size_t target){return combat_position(target);}},
    .select{[&](target_selection_flags flags){return activity->select(observe(), index, flags, std::nullopt, random);}},
    .follow_route{follow},
    .move{[&](movement_request const &request){move_herc(index, request, ratio);}},
    .aim{[&](size_t target, int16_t facing){aim_herc(index, target, facing, ratio);}},
    .centre_aim{[&]{actor.centre_aim(0, ratio);}},
    .path_blocked{[&]{return target_path_blocked(index);}},
    .enter{[&](behavior type){enter_herc(index, type);}},
    .ram_advance{[&]{advance_herc(index, ratio, true);}},
  };
  dispatch_controller(state.controller, member.skip_decision, ratio, {
    .reconsider{[&](behavior type){
      switch(type) {
        case behavior::player: case behavior::in_limbo: case behavior::herc_dead: case behavior::disabled: return;
        case behavior::attacking: case behavior::flanking: case behavior::facing_off: case behavior::attacking_base: case behavior::attacking_flyer: case behavior::fleeing: reconsider_herc(index); return;
        default: resume_herc_orders(index); return;
      }
    }},
    .advance{[&](behavior type){if(type != behavior::in_limbo && type != behavior::herc_deciding) advance_herc(index, ratio, type == behavior::ramming);}},
    .decide{[&](behavior type){
      switch(type) {
        case behavior::herc_deciding: case behavior::in_limbo: return false;
        case behavior::player:
          if(group.members.front() == index && group.route >= 0) {
            // Route arrival is checked here; player controls are prepared later.
            auto cursor{group.route_cursor};
            follow_route(actor.body().movement().position, actor.body().movement().angles.z, 0, routes.at(static_cast<size_t>(group.route)), points, cursor);
            group.route_cursor = cursor;
          }
          return false;
        case behavior::sleeping:
          assign_target(activity->targets(), index, std::nullopt);
          [[fallthrough]];
        case behavior::herc_dead: case behavior::disabled: move_herc(index, {}, ratio); return false;
        case behavior::patrolling: case behavior::search_destroy:
          patrol_engagement(type == behavior::patrolling ? engagement_mode::herc_patrol : engagement_mode::herc_search, state, index, activity->targets(), activity->objective(index), ratio, activity->entry_stagger(), {
            .navigate{[&]{navigate_herc(index, ratio);}}, .select{orders.select}, .attack{[&]{reconsider_herc(index);}}, .enter{orders.enter},
          });
          return false;
        case behavior::travelling: case behavior::bulldog_travel:
          travel_with_herc(member.travel, activity->targets(), index, activity->actor(index).persistent_signature, actor.impacts().detectable, ratio, orders); return false;
        case behavior::attacking: case behavior::flanking: case behavior::facing_off: case behavior::attacking_base: case behavior::attacking_flyer: return attack(type);
        case behavior::fleeing: {
          flee_with_herc(member.flee, activity->targets(), index, state.command, false, false, ratio, random, orders);
          auto const observations{observe()}; auto const target{member.flee.threat.value()};
          return incapacitated(state) || observations[target].dead || observations[target].disabled || observations[target].unavailable;
        }
        case behavior::skirting:
          skirt_with_herc(member.skirt, index, combat_position(activity->targets()[index].selected.value()).position, member.obstructed, ratio, orders); return false;
        case behavior::ramming:
          ram_with_herc(member.ram, activity->targets(), index, ratio, random, orders); return false;
        case behavior::guarding: {
          std::vector<size_t> candidates(crew.size()); std::iota(candidates.begin(), candidates.end(), 0);
          guard_with_herc(activity->decisions(), activity->targets(), index, candidates,
            {guard_anchor(index), activity->actor(group.members.front()).player, member.radar_active, activity->actor(index).persistent_signature, group.representative.has_value(), member.designated}, actor.impacts().detectable, ratio, {
            .position{orders.position}, .eligible{[&](size_t target){return activity->eligible(observe(), index, target);}},
            .navigate{[&]{navigate_herc(index, ratio);}}, .centre_aim{orders.centre_aim},
            .move_to{[&](vec3<int32_t> const &point){auto const own{combat_position(index)}; move_herc(index, {.speed{member.requested_speed ? member.requested_speed : int16_t{170}}, .turn{static_cast<int16_t>(wrap_word(bearing_angle(own.position, point) - own.heading) >> 4)}}, ratio);}},
            .move{orders.move}, .choose_component{[&]{return actor.choose_aimed_component(*crew.at(activity->targets()[index].selected.value()).actor, random);}}, .enter{orders.enter},
          });
          return false;
        }
        case behavior::driving_off_enemy: {
          auto const target{activity->targets()[index].selected.value()};
          return drive_off_enemy(combat_position(index), combat_position(target), guard_anchor(index), member.designated, member.aim_tolerance, {
            .obstruction_entry{[&]{return enter_skirting(index);}}, .face_off{[&]{return attack(behavior::facing_off);}}, .move{orders.move},
            .aim{[&](int16_t facing){aim_herc(index, target, facing, ratio);}}, .target_completed{[&]{return herc_target_completed(index);}}, .incapacitated{[&]{return incapacitated(state);}},
          });
        }
        default: throw std::logic_error{"flyer behavior assigned to HERC"};
      }
    }},
  });
}
} // namespace earthsiege
