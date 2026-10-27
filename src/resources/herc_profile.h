#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <vector>
#include "resources/binary_reader.h"

namespace earthsiege {

struct movement_profile {
  int16_t steering_rate{0};
  int16_t reverse_rate{0};
  int16_t forward_rate{0};
  int16_t acceleration{0};
  int16_t steering_acceleration{0};
  int16_t movement_sequence{0};
  int16_t fast_sequence{0};
  int16_t forward_stop_sequence{0};
  int16_t reverse_stop_sequence{0};
  int16_t gait_threshold{0};
  int16_t reverse_fast_exit_rate{0};
  int16_t turn_sequence{0};
  int16_t disabled_sequence{0};
};

struct aiming_profile {
  int16_t sequence{0};
  int16_t rate{0};
  int16_t acceleration{0};
  int16_t minimum{0};
  int16_t maximum{0};
};

enum class herc_texture_group : int16_t {
  light,
  medium,
  heavy,
  enemy,
};

struct herc_profile {
  movement_profile movement;
  aiming_profile torso;
  aiming_profile elevation;
  int16_t position_height{0};
  std::string name;
  herc_texture_group textures{herc_texture_group::light};
  std::vector<uint8_t> detail_part_ids;
  std::array<uint8_t, 192> source{};                                             // retain fields whose consumers are still being integrated
};

herc_profile read_herc_profile(bytes source);

} // namespace earthsiege
