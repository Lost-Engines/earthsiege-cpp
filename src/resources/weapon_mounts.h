#pragma once
#include <array>
#include <cstdint>
#include <vector>
#include "resources/binary_reader.h"
#include "vectorstorm/vector/vector3.h"

namespace earthsiege {

struct weapon_mount {
  int16_t model_part{0};
  uint8_t orientation{0};
  vec3<int16_t> position;
  int8_t equipment_slot{0};
  std::array<uint8_t, 26> source{};
  bool external() const;
};

std::vector<weapon_mount> read_weapon_mounts(bytes source);

} // namespace earthsiege
