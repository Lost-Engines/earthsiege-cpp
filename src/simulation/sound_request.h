#pragma once
#include <cstdint>
#include <functional>
#include <variant>
#include "vectorstorm/vector/vector3.h"

namespace earthsiege {
enum class sound_action { play, positioned, stop, continuous };
struct sample_sound_request {
  sound_action action;
  uint16_t id;
  vec3<int32_t> position;
  sample_sound_request(sound_action requested, uint16_t sound_id, vec3<int32_t> const &where = {})
    : action{requested}, id{sound_id}, position{where} {}
};
struct servo_sound_request { size_t axis; int16_t control, angle; };
struct target_sound_request { bool locked, changed; };
using sound_request = std::variant<sample_sound_request,servo_sound_request,target_sound_request>;
using sound_sink = std::function<void(sound_request const &)>;
} // namespace earthsiege
