#pragma once
#include <cstdint>
#include "vectorstorm/vector/vector3.h"
#include "resources/herc_profile.h"
#include "simulation/animation_channel.h"

namespace earthsiege {

struct locomotion_state {
  vec3<int32_t> position;
  vec3<uint16_t> angles;
  int16_t throttle{0};
  int16_t gait_rate{0};
  int16_t steering_rate{0};
  animation_rate playback_rate{0};
};

/// Unobstructed normal-player control; collision and disabled states are separate consumers
void update_player_movement(locomotion_state &state, movement_profile const &profile, animation_channel &channel, int16_t throttle_input, int16_t steering_input);
/// Prepared AI speed/turn request, after avoidance and damage modifiers
void update_requested_movement(locomotion_state &state, movement_profile const &profile, animation_channel &channel, int16_t speed, int16_t steering);
void advance_root_movement(locomotion_state &state, animation_channel &channel, int16_t time_ratio);

} // namespace earthsiege
