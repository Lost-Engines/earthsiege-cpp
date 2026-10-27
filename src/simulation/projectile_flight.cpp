#include "projectile_flight.h"
#include <algorithm>
#include <stdexcept>
#include "simulation/fixed_math.h"
#include "simulation/model_math.h"

namespace earthsiege {
namespace {

void guide(projectile_flight &state, projectile_guidance &input, frame_ratio const ratio) {
  if(state.subtype == rocket_manual_subtype && input.player_owner) {
    if(!input.manual_held) {
      state.target_present = false;
      state.subtype = 0;
    } else {
      auto const yaw{wrap_word((rocket_turn_limit * input.manual_axes.x + 128) >> 8)};
      auto const pitch{wrap_word((rocket_turn_limit * input.manual_axes.y + 128) >> 8)};
      state.angles.z = static_cast<uint16_t>(state.angles.z - scale_frame_time(yaw, ratio));
      state.angles.x = static_cast<uint16_t>(state.angles.x + scale_frame_time(pitch, ratio));
      input.manual_axes = {};
      input.manual_held = false;
    }
    return;
  }
  if(!state.target_present || (input.player_owner && !input.owner_tracks_target) || (state.subtype == rocket_target_sensor_subtype && !input.target_detectable)) return;
  vec3<int32_t> const delta{
    wrap_long(static_cast<int64_t>(input.target_position.x) - state.position.x),
    wrap_long(static_cast<int64_t>(input.target_position.y) - state.position.y),
    wrap_long(static_cast<int64_t>(input.target_position.z) - state.position.z),
  };
  auto const horizontal{fast_distance_2d({delta.x, delta.y})};
  vec2<uint16_t> const desired{direction_angle(horizontal, delta.z), static_cast<uint16_t>(direction_angle(delta.x, delta.y) - angle_quarter_turn)};
  if(state.subtype == rocket_manual_subtype) input.owner_guiding = true;
  for(unsigned int i{0}; i != 2; ++i) {
    unsigned int const axis{i == 0 ? 0U : 2U};
    int16_t error{wrap_word(static_cast<int32_t>(desired[i]) - state.angles[axis])};
    if(state.subtype != rocket_manual_subtype && state.subtype != rocket_target_sensor_subtype && input.interference && error >= -rocket_interference_angle && error < rocket_interference_angle) {
      error = wrap_word(error + (error < 0 ? rocket_interference_angle : -rocket_interference_angle));
    }
    auto const turn{scale_frame_time(std::clamp<int16_t>(error, -rocket_turn_limit, rocket_turn_limit), ratio)};
    state.angles[axis] = static_cast<uint16_t>(state.angles[axis] + turn);
  }
}

} // namespace

projectile_result advance_projectile(projectile_flight &state, projectile_motions const &motions, int16_t const speed_limit, std::span<uint16_t const> const cell_counts, frame_ratio const ratio, projectile_guidance &guidance, projectile_contact const &contact) {
  int16_t lifetime, radius, period, sequence{0}, acceleration{0};
  if(state.kind == projectile_kind::bullet) {
    auto const &motion{motions.bullets.at(state.subtype)};
    lifetime = motion.lifetime; radius = motion.collision_radius; period = motion.animation_period;
  } else if(state.kind == projectile_kind::rocket) {
    auto const &motion{motions.rockets.at(state.subtype)};
    lifetime = motion.lifetime; radius = motion.collision_radius; period = motion.animation_period;
    sequence = motion.cell_sequence; acceleration = motion.acceleration;
  } else throw std::invalid_argument{"projectile kind has no bullet/rocket flight"};
  if(period != 0) {
    state.animation_remaining = std::max<int16_t>(0, wrap_word(state.animation_remaining - ratio));
    if(state.animation_remaining == 0) {
      auto const cell{static_cast<size_t>(sequence)};
      if(cell >= cell_counts.size() || cell >= state.cells.size() || cell_counts[cell] == 0) throw std::out_of_range{"projectile cell sequence"};
      state.animation_remaining = period;
      state.cells[cell] = static_cast<uint16_t>((state.cells[cell] + 1U) % cell_counts[cell]);
    }
  }
  state.age = wrap_word(state.age + 1);
  if(state.age > lifetime) {
    if(state.kind == projectile_kind::rocket && state.subtype == rocket_manual_subtype) guidance.manual_held = false;
    return projectile_result::expired;
  }
  if(state.kind == projectile_kind::rocket) {
    auto const accelerated{wrap_word(state.speed + scale_frame_time(acceleration, ratio))};
    state.speed = std::min(speed_limit, wrap_word((state.speed + accelerated) >> 1));
    guide(state, guidance, ratio);
  }
  auto transform{rotation_transform(state.angles)};
  transform.translation = state.position;
  auto const distance{scale_frame_time(state.speed, ratio)};
  if(contact && contact(transform, distance, radius)) {
    if(state.kind == projectile_kind::rocket && state.subtype == rocket_manual_subtype) guidance.manual_held = false;
    return projectile_result::impact;
  }
  state.position = transform_point({0, distance, 0}, transform);
  return projectile_result::flying;
}

} // namespace earthsiege
