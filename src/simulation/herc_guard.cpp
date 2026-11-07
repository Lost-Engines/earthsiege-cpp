#include "herc_guard.h"
#include <algorithm>
#include <stdexcept>
#include "simulation/fixed_math.h"
#include "simulation/model_math.h"

namespace earthsiege {
namespace {
int32_t distance(vec3<int32_t> const &a, vec3<int32_t> const &b) {
  return fast_distance_2d({wrap_long(static_cast<int64_t>(a.x) - b.x), wrap_long(static_cast<int64_t>(a.y) - b.y)});
}
bool incapacitated(engagement_state const &actor) { return actor.dead || actor.disabled || actor.unavailable; }
}
void guard_with_herc(std::span<engagement_state> const actors, std::span<target_relationship> const targets, size_t const actor, std::span<size_t const> const candidates, herc_guard_policy const &policy, bool &detectable, frame_ratio const ratio, herc_guard_callbacks const &callbacks) {
  auto &state{actors[actor]};
  assign_target(targets, actor, std::nullopt);
  auto &timer{state.controller.target_search_remaining};
  bool const following_player{policy.leader_player && state.command == 0};
  bool const may_search{!policy.expansion || !following_player || policy.weapons_free};
  if(may_search) timer = std::max<int16_t>(0, wrap_word(timer - ratio));
  // Expansion 0x19f34 keeps guarding the anchor between searches. Base
  // 0x17ed4 instead delegates those frames to ordinary route navigation.
  if(!policy.expansion && (timer != 0 || following_player)) { callbacks.navigate(); return; }
  std::optional<size_t> selected;
  if(may_search && timer == 0) {
  if(state.command == 4 && policy.designated) selected = policy.designated;
  else {
    int32_t const radius{policy.group_has_objective ? 945000 : 980000};
    timer = 10000;
    bool best_active{false}; int32_t best{0};
    for(auto const candidate : candidates) {
      if(!callbacks.eligible(candidate) || actors[candidate].disabled) continue;
      bool const active{!incapacitated(actors[candidate])};
      if(best_active && !active) continue;
      auto score{wrap_long(static_cast<int64_t>(radius) - distance(policy.anchor, callbacks.position(candidate).position))};
      if(score <= 0) continue;
      auto const attackers{targets[candidate].attackers};
      if(attackers == -1) throw std::logic_error{"inconsistent guard target attacker count"};
      if(attackers != 0) score /= attackers + 1;
      if(score > best || (!best_active && active)) { best = score; best_active = active; selected = candidate; }
    }
  }
  }
  if(!selected) {
    if(following_player) {
      callbacks.navigate();
      detectable = policy.radar_active;
      callbacks.centre_aim();
      return;
    }
    auto const own{callbacks.position(actor)};
    auto const range{distance(policy.anchor, own.position)};
    callbacks.centre_aim();
    detectable = policy.leader_player ? policy.radar_active : policy.persistent_signature;
    if(range > 20000) { callbacks.move_to(policy.anchor); return; }
    auto const error{wrap_word(bearing_angle(own.position, policy.anchor) - own.heading - 32768)};
    callbacks.move({.speed{static_cast<int16_t>(range < 10000 ? 256 : 0)}, .turn{static_cast<int16_t>(error >> 4)}});
    return;
  }
  assign_target(targets, actor, selected);
  if(incapacitated(state)) {
    callbacks.enter(behavior::fleeing);
    state.aimed_component = -1;
  } else {
    callbacks.enter(behavior::driving_off_enemy);
    state.aimed_component = callbacks.choose_component();
    if(!policy.leader_player || policy.radar_active) detectable = true;
  }
}

bool drive_off_enemy(herc_combat_position const &actor, herc_combat_position const &target, vec3<int32_t> const &anchor, std::optional<size_t> &designated, int16_t &aim_tolerance, herc_drive_callbacks const &callbacks) {
  auto const target_range{distance(anchor, target.position)}, own_range{distance(anchor, actor.position)};
  if(callbacks.obstruction_entry()) return false;
  auto const stand_off{std::min(target_range >> 1, 50000)};
  auto const heading{bearing_angle(anchor, target.position)};
  auto point{rotate_vector({0, stand_off, 0}, rotation_transform({0, 0, heading}))};
  point.x = wrap_long(static_cast<int64_t>(point.x) + anchor.x);
  point.y = wrap_long(static_cast<int64_t>(point.y) + anchor.y);
  auto const error{wrap_word(bearing_angle(actor.position, point) - actor.heading)};
  auto const target_error{wrap_word(bearing_angle(actor.position, target.position) - actor.heading)};
  auto const difference{wrap_word(error - target_error)};
  if(target_range <= stand_off && ((difference < -4096 || difference >= 4096) || target_range <= own_range)) return callbacks.face_off();
  if(distance(point, actor.position) < 5000) callbacks.move({.turn{static_cast<int16_t>(target_error >> 4)}});
  else callbacks.move({.speed{static_cast<int16_t>(error >= -8192 && error < 8192 ? 256 : -256)}, .turn{static_cast<int16_t>(error >> 4)}});
  aim_tolerance = 750;
  callbacks.aim(wrap_word(bearing_angle(target.position, actor.position) - target.heading + target.view_offset));
  if(callbacks.target_completed() || callbacks.incapacitated()) { designated.reset(); return true; }
  return false;
}
} // namespace earthsiege
