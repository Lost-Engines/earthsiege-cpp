#pragma once
#include <optional>
#include "simulation/sound_request.h"
namespace earthsiege {
struct servo_sound_state { bool active{}; int16_t previous{}; };
struct target_sound_state { bool pulse{}, previous_lock{}; };
std::optional<sound_action> advance_servo_sound(servo_sound_state &state, int16_t control, int16_t angle, bool playing);
std::optional<uint16_t> advance_target_sound(target_sound_state &state, bool locked, bool changed, uint32_t clock);
std::optional<sound_action> engine_sound_gate(bool external_view, bool engine_playing, bool startup_playing);
} // namespace earthsiege
