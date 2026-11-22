#pragma once
#include <span>
#include "resources/model.h"
namespace earthsiege {
struct development_controls {
  int16_t translation_step{2000};
  int16_t turn_step{2000};
  int16_t movement_direction{0};
  bool reposition(uint16_t command, vec3<int32_t> &position, vec3<uint16_t> &angles);
  void toggle_direction() {
    movement_direction = movement_direction <= 0 ? 1 : -1;
  }
};
struct development_contact {
  vec3<int32_t> position;
  int32_t radius{0};
  int16_t strength{0};
  uint8_t side{0};
  bool eligible{false};
};
vec3<int32_t> development_pod_destination(vec3<int32_t> const &position, uint16_t heading);
int development_damage_passes(std::span<development_contact const> contacts, vec3<int32_t> const &position, uint8_t side, bool safe);
} // namespace earthsiege
