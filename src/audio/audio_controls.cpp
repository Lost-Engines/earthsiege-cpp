#include "audio_controls.h"
#include "simulation/fixed_math.h"

namespace earthsiege {

std::optional<sound_action> advance_servo_sound(servo_sound_state &state, int16_t const control, int16_t const angle, bool const playing) {
  auto const magnitude{control < 0 ? wrap_word(-static_cast<int32_t>(control)) : control};
  bool const angle_changed{state.previous != angle};
  std::optional<sound_action> result;

  if(!state.active && !playing && magnitude > 192 && angle_changed) {
    result = sound_action::play;
    state.active = true;
  } else if(state.active && (magnitude == 0 || !angle_changed)) {
    result = sound_action::stop;
    state.active = false;
  }

  state.previous = angle;
  return result;
}

std::optional<uint16_t> advance_target_sound(target_sound_state &state, bool const locked, bool const changed, uint32_t const clock) {
  if(locked) {
    state.previous_lock = true;
    if((clock & 0b0100'0000u) == 0) {
      state.pulse = false;
    } else if(!state.pulse) {
      state.pulse = true;
      return 21;
    }
    return std::nullopt;
  }

  if(state.previous_lock) {
    state.previous_lock = false;
    return 22;
  }
  if(changed) return 20;
  return std::nullopt;
}

std::optional<sound_action> engine_sound_gate(bool const external_view, bool const engine_playing, bool const startup_playing) {
  if(external_view && engine_playing) return sound_action::stop;
  if(!external_view && !engine_playing && !startup_playing) return sound_action::play;
  return std::nullopt;
}

} // namespace earthsiege
