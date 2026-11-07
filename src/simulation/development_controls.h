#pragma once
#include <span>
#include "resources/model.h"
namespace earthsiege {
struct development_controls {
  int16_t translation_step{2000},turn_step{2000},movement_direction{};
  bool reposition(uint16_t command,vec3<int32_t> &position,vec3<uint16_t> &angles);
  void toggle_direction(){movement_direction=movement_direction<=0?1:-1;}
};
struct development_contact {
  vec3<int32_t> position;
  int32_t radius{};
  int16_t strength{};
  uint8_t side{};
  bool eligible{};
};
vec3<int32_t> development_pod_destination(vec3<int32_t> const &position,uint16_t heading);
int development_damage_passes(std::span<development_contact const> contacts,vec3<int32_t> const &position,uint8_t side,bool safe);
}
