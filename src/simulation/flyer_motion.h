#pragma once
#include "resources/flyer_definition.h"
#include "simulation/frame_time.h"
#include "simulation/navigation.h"

namespace earthsiege {
struct flyer_motion {
  vec3<int32_t> position;
  vec3<uint16_t> angles;
  vec3<int16_t> velocity;
  vec3<int16_t> angular_step;
  int16_t obstacle_strength{0};
  int16_t obstacle_timer{0};
  bool obstacle{false};
  int16_t previous_yaw_step{}; // expansion: yaw saved before local matrix integration
};
struct flyer_leader_turn {
  uint16_t heading{0};
  int16_t heading_step{0};
};
void aim_flyer_pitch(flyer_motion &state, flyer_profile const &profile, int16_t pitch);
void steer_flyer(flyer_motion &state, flyer_profile const &profile, int16_t pitch_input, int16_t bank_input);
void hold_flyer_bank(flyer_motion &state, flyer_profile const &profile, int16_t bank);
void turn_flyer(flyer_motion &state, flyer_profile const &profile, int16_t heading_error, std::optional<flyer_leader_turn> leader = {});
void hold_flyer_altitude(flyer_motion &state, flyer_profile const &profile, int32_t altitude);
/// Translation and Euler steps are per dispatch; only the obstacle clock uses elapsed time.
void advance_flyer(flyer_motion &state, flyer_profile const &profile, frame_ratio ratio, avoidance_query const &query);
model_transform flyer_placement(flyer_motion const &state);
} // namespace earthsiege
