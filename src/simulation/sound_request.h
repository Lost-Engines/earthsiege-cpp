#pragma once
#include <cstdint>
#include <functional>
#include "vectorstorm/vector/vector3.h"

namespace earthsiege {
enum class sound_action { play, positioned, stop, continuous };
struct sound_request {
  sound_action action;
  uint16_t id;
  vec3<int32_t> position;
  sound_request(sound_action requested, uint16_t sound_id, vec3<int32_t> const &where = {})
    : action{requested}, id{sound_id}, position{where} {}
};
using sound_sink = std::function<void(sound_request const &)>;
} // namespace earthsiege
