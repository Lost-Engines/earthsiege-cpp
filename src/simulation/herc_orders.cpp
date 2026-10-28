#include "herc_orders.h"
#include <algorithm>
#include <stdexcept>
#include "simulation/fixed_math.h"
#include "simulation/model_math.h"

namespace earthsiege {
namespace {
void count_down(int32_t &value, frame_ratio ratio) { value = std::max(0, wrap_long(static_cast<int64_t>(value) - ratio)); }
void count_down(int16_t &value, frame_ratio ratio) { value = std::max<int16_t>(0, wrap_word(value - ratio)); }
}
void travel_with_herc(herc_travel_state &state, std::span<target_relationship> const targets, size_t const actor, bool const persistent_signature, bool &detectable, frame_ratio const ratio, herc_order_callbacks const &callbacks) {
  callbacks.follow_route();
  assign_target(targets, actor, std::nullopt);
  count_down(state.search_remaining, ratio);
  if(state.search_remaining == 0) {
    state.threat = callbacks.select(target_selection_flags::none);
    state.search_remaining = 10000;
    detectable = state.threat.has_value() || persistent_signature;
  }
  if(state.threat) {
    auto const own{callbacks.position(actor)}, target{callbacks.position(*state.threat)};
    callbacks.aim(*state.threat, wrap_word(bearing_angle(target.position, own.position) - target.heading + target.view_offset));
  }
}

bool flee_with_herc(herc_flee_state &state, std::span<target_relationship> const targets, size_t const actor, int16_t &command, bool const incapacitated, bool const threat_incapacitated, frame_ratio const ratio, random_state &random, herc_order_callbacks const &callbacks) {
  if(targets[actor].selected) {
    state.threat = targets[actor].selected;
    assign_target(targets, actor, std::nullopt);
    command = 0;
  }
  if(!state.threat) throw std::logic_error{"fleeing HERC has no remembered threat"};
  auto geometry{relative_herc_target(callbacks.position(actor), callbacks.position(*state.threat))};
  if(geometry.bearing_magnitude < 12288) {
    geometry.direction = -1;
    geometry.bearing_error = wrap_word(geometry.bearing_error - 32768);
  } else {
    count_down(state.turn_remaining, ratio);
    if(state.turn_remaining == 0) { state.turn_remaining = 4000; state.turn_right = (random.next() & 1) != 0; }
    geometry.direction = 1;
    geometry.bearing_error = wrap_word(geometry.bearing_error + (state.turn_right ? -24576 : 24576));
  }
  geometry.near_distance = 0; geometry.far_distance = 10000000;
  int16_t direction{0};
  auto const request{herc_range_movement(geometry, direction)};
  // Shared scratch +0x54 is a direction word, but this controller's long timer
  // starts at +0x55. The direction write replaces its low byte every update.
  state.turn_remaining = wrap_long((static_cast<uint32_t>(state.turn_remaining) & 0xffffff00U) | (static_cast<uint16_t>(direction) >> 8));
  callbacks.move(request);
  callbacks.aim(*state.threat, geometry.facing_error);
  return incapacitated || threat_incapacitated;
}

void skirt_with_herc(herc_skirt_state &state, size_t const actor, vec3<int32_t> const &obstacle, bool &obstruction, frame_ratio const ratio, herc_order_callbacks const &callbacks) {
  auto const own{callbacks.position(actor)};
  auto const radial{bearing_angle(obstacle, own.position)};
  if(!state.initialized) {
    state.turn_left = wrap_word(bearing_angle(own.position, obstacle) - own.heading) < 0;
    state.initialized = true;
    state.check_remaining = 5000;
  }
  count_down(state.check_remaining, ratio);
  if(state.check_remaining == 0) {
    state.check_remaining = 5000;
    if(!callbacks.path_blocked()) {
      if(!state.previous) throw std::logic_error{"skirting HERC has no previous behavior"};
      callbacks.enter(*state.previous);
      state = {};                                                           // original entry clears scratch before the remaining movement below
      obstruction = false;
    }
  }
  auto const heading{static_cast<uint16_t>(radial + (state.turn_left ? -16384 : 16384))};
  auto point{rotate_vector({0, state.distance, 0}, rotation_transform({0, 0, heading}))};
  for(unsigned int axis{0}; axis != 3; ++axis) point[axis] = wrap_long(static_cast<int64_t>(point[axis]) + obstacle[axis]);
  callbacks.move({.speed{256}, .turn{static_cast<int16_t>(wrap_word(bearing_angle(own.position, point) - own.heading) >> 4)}});
  callbacks.centre_aim();
}

void ram_with_herc(herc_ram_state &state, std::span<target_relationship> const targets, size_t const actor, frame_ratio const ratio, random_state &random, herc_order_callbacks const &callbacks) {
  count_down(state.search_remaining, ratio);
  if(state.search_remaining == 0) {
    assign_target(targets, actor, callbacks.select(static_cast<target_selection_flags>(6)));
    state.search_remaining = 10000;
  }
  auto const selected{targets[actor].selected};
  if(!selected) { callbacks.move({.flags{1}}); return; }
  auto const steer{[&]{
    auto const own{callbacks.position(actor)}, target{callbacks.position(*selected)};
    callbacks.move({.flags{1}, .speed{256}, .turn{static_cast<int16_t>(wrap_word(bearing_angle(own.position, target.position) - own.heading) >> 4)}});
  }};
  steer();
  if(state.bursting) {
    callbacks.ram_advance(); steer();
    callbacks.ram_advance(); steer();
    count_down(state.burst_remaining, ratio);
    if(state.burst_remaining == 0) { state.bursting = false; state.burst_remaining = wrap_word(8000 + random.bounded(4000)); }
  } else {
    count_down(state.burst_remaining, ratio);
    if(state.burst_remaining == 0) { state.bursting = true; state.burst_remaining = wrap_word(3000 + random.bounded(1500)); }
  }
}
} // namespace earthsiege
