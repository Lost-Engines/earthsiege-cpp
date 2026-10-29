#pragma once
#include <span>
#include "resources/herc_profile.h"
#include "simulation/animation_channel.h"

namespace earthsiege {
struct footstep_attachment { int8_t type{}; bool active{}; int32_t local_y{}, world_z{}; };
struct footstep_state { std::array<uint8_t,5> latches{0,0,0,0,1}; };
/// Original 0x144dc: authored gait thresholds, reverse gait and collapse latch.
std::vector<uint16_t> advance_footsteps(footstep_state &state, movement_profile const &movement,
  std::span<int16_t const,10> trigger, std::span<int16_t const,10> rearm,
  animation_channel_state const &channel, int16_t rate, bool player,
  std::span<footstep_attachment const> attachments);
} // namespace earthsiege
