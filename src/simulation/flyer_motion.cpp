#include "flyer_motion.h"
#include <algorithm>
#include <stdexcept>
#include "simulation/fixed_math.h"
#include "simulation/model_math.h"

namespace earthsiege {
namespace {
int16_t approach(int16_t value, int16_t const target, int16_t const step) {
  if(target < value) { value = wrap_word(value - step); if(value <= target) value = target; }
  else if(value < target) { value = wrap_word(value + step); if(target <= value) value = target; }
  return value;
}
int16_t scaled(int32_t const value, int16_t const scale, unsigned int const bits) { return wrap_word(wrap_long((static_cast<int64_t>(value) * scale + (1 << (bits - 1))) >> bits)); }
}
model_transform flyer_placement(flyer_motion const &state) {
  auto result{rotation_transform(state.angles)}; result.translation = state.position; return result;
}
void aim_flyer_pitch(flyer_motion &state, flyer_profile const &profile, int16_t const pitch) {
  state.angles.x = static_cast<uint16_t>(approach(wrap_word(state.angles.x),pitch,profile.pitch_step));
}
void steer_flyer(flyer_motion &state, flyer_profile const &profile, int16_t const pitch_input, int16_t const bank_input) {
  auto const input{std::clamp<int16_t>(bank_input,-256,256)};
  state.angular_step.y = approach(state.angular_step.y,scaled(input,profile.bank_rate,8),profile.bank_acceleration);
  state.angular_step.z = wrap_word(-static_cast<int32_t>(wrap_word(state.angles.y)) >> (profile.bank_turn_shift & 31));
  aim_flyer_pitch(state,profile,wrap_word(state.angles.x + pitch_input * 16));
}
void hold_flyer_bank(flyer_motion &state, flyer_profile const &profile, int16_t const bank) {
  steer_flyer(state,profile,0,wrap_word((bank - wrap_word(state.angles.y)) >> 4));
}
void turn_flyer(flyer_motion &state, flyer_profile const &profile, int16_t const heading_error, std::optional<flyer_leader_turn> const leader) {
  auto yaw{state.angular_step.z};
  if(leader) {
    auto const projected{scaled(leader->heading_step,model_cosine(static_cast<uint16_t>(state.angles.z - leader->heading)),14)};
    yaw = wrap_word(yaw - scaled(200,projected,10));
  }
  auto const input{wrap_word((-static_cast<int32_t>(heading_error) >> 4) + scaled(1100,yaw,10))};
  auto const bank{wrap_word(state.angles.y)};
  if(bank >= profile.maximum_bank - 500 && input > 0) hold_flyer_bank(state,profile,profile.maximum_bank);
  else if(bank <= -(profile.maximum_bank - 500) && input < 0) hold_flyer_bank(state,profile,wrap_word(-profile.maximum_bank));
  else steer_flyer(state,profile,0,input);
}
void hold_flyer_altitude(flyer_motion &state, flyer_profile const &profile, int32_t altitude) {
  if(state.obstacle) altitude = wrap_long((static_cast<int64_t>(15000) * state.obstacle_strength + 128) >> 8);
  aim_flyer_pitch(state,profile,wrap_word(direction_angle(6000,wrap_long(static_cast<int64_t>(altitude) - state.position.z))));
}
void advance_flyer(flyer_motion &state, flyer_profile const &profile, frame_ratio const ratio, avoidance_query const &query) {
  state.velocity.y = profile.forward_step;
  state.position = transform_point(state.velocity,flyer_placement(state));
  for(unsigned int axis{0}; axis != 3; ++axis) state.angles[axis] = static_cast<uint16_t>(state.angles[axis] + state.angular_step[axis]);
  auto end{transform_point({0,15000,0},flyer_placement(state))};
  auto start{state.position}; end.z = wrap_long(static_cast<int64_t>(end.z) - 500); start.z = wrap_long(static_cast<int64_t>(start.z) - 500);
  int32_t distance{0};
  if(end.z < 0) {
    if(start.z < 0) distance = 1;
    else {
      vec3<int32_t> delta;
      for(unsigned int axis{0}; axis != 3; ++axis) delta[axis] = wrap_long(static_cast<int64_t>(end[axis]) - start[axis]);
      auto const downward{-static_cast<int32_t>(wrap_word(direction_angle(fast_distance_2d({delta.x,delta.y}),delta.z)))};
      if(downward != 0) {
        auto const sine{model_cosine(static_cast<uint16_t>(downward - angle_quarter_turn))};
        if(sine == 0) throw std::runtime_error{"original flyer ground probe divides by zero"};
        distance = wrap_long(static_cast<int64_t>(start.z) * model_matrix_one) / sine;
      }
    }
  }
  // Original compares both object hits against zero when no ground intersection
  // exists, so positive object distances alone do not start avoidance.
  if(query) for(auto const kind : {avoidance_probe::scenery,avoidance_probe::ground_objects}) if(auto const hit{query(kind,start,end)}; hit && *hit < distance) distance = *hit;
  state.obstacle_timer = std::max<int16_t>(0,wrap_word(state.obstacle_timer - ratio));
  state.obstacle = state.obstacle_timer != 0 || distance != 0;
  if(distance != 0) {
    state.obstacle_strength = wrap_word(wrap_long(static_cast<int64_t>(15000 - distance) * 256) / 15000);
    state.obstacle_timer = 1500;
  }
}
} // namespace earthsiege
