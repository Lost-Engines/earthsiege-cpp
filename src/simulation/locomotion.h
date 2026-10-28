#pragma once
#include <cstdint>
#include <functional>
#include "vectorstorm/vector/vector3.h"
#include "resources/herc_profile.h"
#include "simulation/animation_channel.h"
#include "simulation/frame_time.h"

namespace earthsiege {

struct locomotion_state {
  vec3<int32_t> position;
  vec3<uint16_t> angles;
  int16_t throttle{0};
  int16_t gait_rate{0};
  int16_t steering_rate{0};
  animation_rate playback_rate{0};
  int32_t collision_recovery{0};
  bool reverse_recovery{false};
  bool collapsed{false};
};

struct movement_damage {
  bool disabled{false};
  bool light{false};
  bool heavy{false};
};

enum class movement_event { collapse_started, collapsed };
using movement_event_sink = std::function<void(movement_event)>;
using movement_adjustment = std::function<void(int16_t &, int16_t &)>;

/// Player throttle preparation followed by the shared gait/damage controller
void update_player_movement(locomotion_state &state, movement_profile const &profile, animation_channel &channel, int16_t throttle_input, int16_t steering_input, movement_damage const &damage = {}, frame_ratio ratio = frame_ratio_one, movement_event_sink const &events = {});
/// Prepared AI speed/turn request; apply recovery, damage modifiers and gait transitions
void update_requested_movement(locomotion_state &state, movement_profile const &profile, animation_channel &channel, int16_t speed, int16_t steering, movement_damage const &damage = {}, frame_ratio ratio = frame_ratio_one, movement_event_sink const &events = {}, movement_adjustment const &avoidance = {});
void advance_root_movement(locomotion_state &state, animation_channel &channel, int16_t time_ratio);

} // namespace earthsiege
