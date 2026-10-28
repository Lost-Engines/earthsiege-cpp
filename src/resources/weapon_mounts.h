#pragma once
#include <array>
#include <cstdint>
#include <vector>
#include "resources/binary_reader.h"
#include "vectorstorm/vector/vector3.h"

namespace earthsiege {

struct weapon_definition;

struct weapon_mount {
  int16_t model_part{0};
  int16_t pitch_sequence{-1};
  int16_t yaw_sequence{-1};
  uint8_t orientation{0};
  int8_t control_number{0};                                                // numbered cockpit control, zero based
  int8_t paired_slot_offset{0};                                            // relative GL slot; zero means unpaired
  vec3<int16_t> position;
  int8_t equipment_slot{0};
  std::array<uint8_t, 26> source{};
  bool external() const;
};

vec3<int16_t> weapon_mount_offset(weapon_mount const &mount, weapon_definition const &definition);
std::vector<weapon_mount> read_weapon_mounts(bytes source);

} // namespace earthsiege
