#pragma once

#include <cstdint>
#include <optional>
#include "simulation/sound_request.h"

namespace earthsiege {

struct servo_sound_state {
  bool active{false};
  int16_t previous{0};
};

struct target_sound_state {
  bool pulse{false};
  bool previous_lock{false};
};

std::optional<sound_action> advance_servo_sound(servo_sound_state &state, int16_t control, int16_t angle, bool playing);
std::optional<uint16_t> advance_target_sound(target_sound_state &state, bool locked, bool changed, uint32_t clock);
std::optional<sound_action> engine_sound_gate(bool external_view, bool engine_playing, bool startup_playing);

} // namespace earthsiege
