#pragma once
#include <span>
#include "resources/herc_profile.h"
#include "simulation/animation_channel.h"
#include "simulation/model_math.h"

namespace earthsiege {
// FLAT.DTS shape attached to an articulated HERC part (original 144dc/11988).
struct herc_ground_attachment {
  int8_t shape{-1};
  bool active{false};
  vec3<int32_t> position{0,0,-100000};
  uint16_t heading{};
};
void place_ground_attachment(herc_ground_attachment &attachment,
  vec3<int32_t> const &local, model_transform const &actor, uint16_t heading);
struct footstep_attachment { int8_t type{}; bool active{}; int32_t local_y{}, world_z{}; };
struct footstep_state { std::array<uint8_t,5> latches{0,0,0,0,1}; };
/// Original 0x144dc: authored gait thresholds, reverse gait and collapse latch.
std::vector<uint16_t> advance_footsteps(footstep_state &state, movement_profile const &movement,
  std::span<int16_t const,10> trigger, std::span<int16_t const,10> rearm,
  animation_channel_state const &channel, int16_t rate, bool player,
  std::span<footstep_attachment const> attachments);
} // namespace earthsiege
