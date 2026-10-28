#include "herc_tactics.h"
#include <algorithm>
#include <cstdlib>
#include <stdexcept>
#include "simulation/fixed_math.h"
#include "simulation/model_math.h"

namespace earthsiege {
namespace {
int16_t magnitude(int16_t value) {
  return static_cast<int16_t>(std::min(std::abs(static_cast<int>(value)), 32767));
}
vec3<int32_t> offset_point(vec3<int32_t> const &origin, int32_t distance, uint16_t heading) {
  auto delta{rotate_vector({0, distance, 0}, rotation_transform({0, 0, heading}))};
  for(unsigned int axis{0}; axis != 3; ++axis) delta[axis] = wrap_long(static_cast<int64_t>(origin[axis]) + delta[axis]);
  return delta;
}
void flank(herc_attack_state &state, herc_target_geometry &geometry, herc_attack_inputs const &input, frame_ratio ratio, herc_attack_callbacks const &callbacks) {
  state.reverse_remaining = std::max<int16_t>(0, wrap_word(state.reverse_remaining - ratio));
  if(state.reverse_remaining != 0) {
    geometry.near_distance = 0;
    geometry.direction = -1;
    geometry.bearing_error = 0;
    state.behind_target = false;
    state.damage_check_remaining = 4000;
  } else {
    state.damage_check_remaining = std::max<int16_t>(0, wrap_word(state.damage_check_remaining - ratio));
    if(state.damage_check_remaining == 0 && input.received_damage > 100) state.reverse_remaining = 1000;
    int16_t const rear_limit{static_cast<int16_t>(state.behind_target ? 16384 : 24576)};
    if(geometry.facing_magnitude < rear_limit) {
      geometry.near_distance = 0;
      geometry.direction = 1;
      auto const heading{static_cast<uint16_t>(geometry.bearing - 32768 + (geometry.bearing_error < 0 ? -6000 : 6000))};
      auto const point{offset_point(input.target.position, 15000, heading)};
      geometry.bearing_error = wrap_word(bearing_angle(input.actor.position, point) - input.actor.heading);
      state.behind_target = false;
    } else {
      state.behind_target = true;
      if(input.actor.view_offset >= -2000 && input.actor.view_offset < 2000) {
        geometry.direction = 0;
        state.steering_memory = 0;
      } else geometry.direction = -1;
    }
  }
  callbacks.move(herc_range_movement(geometry, state.last_direction));
}
}

herc_target_geometry relative_herc_target(herc_combat_position const &actor, herc_combat_position const &target) {
  herc_target_geometry result;
  result.bearing = wrap_word(bearing_angle(actor.position, target.position));
  result.bearing_error = wrap_word(result.bearing - actor.heading);
  result.bearing_magnitude = magnitude(result.bearing_error);
  result.facing_error = wrap_word(result.bearing - 32768 - target.heading + target.view_offset);
  result.facing_magnitude = magnitude(result.facing_error);
  vec3<int32_t> delta;
  for(unsigned int axis{0}; axis != 3; ++axis) delta[axis] = wrap_long(static_cast<int64_t>(target.position[axis]) - actor.position[axis]);
  result.distance = fast_distance(delta);
  return result;
}

movement_request herc_range_movement(herc_target_geometry &geometry, int16_t &last_direction) {
  // Deliberately use the previous magnitude for the range correction, then
  // refresh it: approach/flanking can change bearing_error before this call.
  if(geometry.distance < geometry.near_distance) geometry.direction = geometry.bearing_magnitude < 16384 ? -1 : 1;
  else if(geometry.distance > geometry.far_distance) geometry.direction = geometry.bearing_magnitude <= 16384 ? 1 : -1;
  geometry.bearing_magnitude = magnitude(geometry.bearing_error);
  if(geometry.direction != 0) last_direction = geometry.direction;
  return {.speed{static_cast<int16_t>(geometry.direction < 0 ? -256 : geometry.direction > 0 ? 256 : 0)}, .turn{static_cast<int16_t>(geometry.bearing_error >> 4)}};
}

bool attack_with_herc(behavior const type, herc_attack_state &state, int16_t &command, herc_attack_inputs const &input, frame_ratio const ratio, herc_attack_callbacks const &callbacks) {
  auto geometry{relative_herc_target(input.actor, input.target)};
  switch(type) {
    case behavior::attacking:
      if(geometry.facing_magnitude < 8000) {
        geometry.direction = 32768 - geometry.bearing_magnitude < 4000 ? -1 : 1;
        auto const side{(geometry.bearing_error < 0) == (geometry.bearing_magnitude > 16384) ? 16384 : -16384};
        auto point{offset_point(input.target.position, 18000, static_cast<uint16_t>(input.target.heading + side))};
        if(geometry.direction == -1)
          for(unsigned int axis{0}; axis != 3; ++axis) point[axis] = wrap_long(2 * static_cast<int64_t>(input.actor.position[axis]) - point[axis]);
        geometry.bearing_error = wrap_word(bearing_angle(input.actor.position, point) - input.actor.heading);
      } else geometry.direction = geometry.bearing_magnitude < 8192 ? 0 : -1;
      callbacks.move(herc_range_movement(geometry, state.last_direction));
      break;
    case behavior::flanking:
      flank(state, geometry, input, ratio, callbacks);
      break;
    case behavior::facing_off:
      geometry.direction = geometry.bearing_magnitude < 8192 ? 1 : -1;
      geometry.near_distance = 16000;
      geometry.far_distance = 35000;
      callbacks.move(herc_range_movement(geometry, state.last_direction));
      break;
    case behavior::attacking_base:
      if(!input.base_can_retaliate) geometry.facing_error = geometry.facing_magnitude = 32000;
      flank(state, geometry, input, ratio, callbacks);
      geometry.near_distance = 25000;
      // Original immediate is 0xffffafc8: 45,000 sign-extended from a word.
      // It really is negative; the original also applies locomotion twice here.
      geometry.far_distance = -20536;
      callbacks.move(herc_range_movement(geometry, state.last_direction));
      break;
    case behavior::attacking_flyer:
      geometry.direction = geometry.bearing_magnitude < 2000 && (geometry.facing_magnitude > 2000 || geometry.distance > 35000) ? 0 : -1;
      geometry.near_distance = 0;
      geometry.far_distance = 1000000;
      callbacks.move(herc_range_movement(geometry, state.last_direction));
      break;
    default: throw std::invalid_argument{"not a HERC attack movement controller"};
  }
  callbacks.aim(geometry.facing_error);
  bool const finished{callbacks.completed ? callbacks.completed() : input.actor_incapacitated || ((type == behavior::attacking_base || type == behavior::attacking_flyer) ? input.target_incapacitated : input.target_completed)};
  if(finished && command == 4) command = 0;
  return finished;
}
} // namespace earthsiege
