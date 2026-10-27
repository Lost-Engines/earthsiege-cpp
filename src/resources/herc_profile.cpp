#include "herc_profile.h"
#include <algorithm>

namespace earthsiege {

herc_profile read_herc_profile(bytes const source) {
  binary_reader file{source, "HERC profile"};
  herc_profile result;
  auto const retained{file.take(result.source.size())};
  std::ranges::copy(retained, result.source.begin());
  file.require_end();
  auto const word{[&](size_t const offset){ file.seek(offset); return file.integer<int16_t>(); }};
  result.movement = {
    .steering_rate{word(0)},
    .reverse_rate{word(2)},
    .forward_rate{word(4)},
    .acceleration{word(6)},
    .steering_acceleration{word(8)},
    .movement_sequence{word(12)},
    .fast_sequence{word(14)},
    .forward_stop_sequence{word(16)},
    .reverse_stop_sequence{word(18)},
    .gait_threshold{word(44)},
    .reverse_fast_exit_rate{word(108)},
    .turn_sequence{word(122)},
    .disabled_sequence{word(68)},
  };
  result.position_height = word(20);
  result.collision_radius = word(110);
  result.mass_factor = word(76);
  result.camera_part = word(10);
  result.camera_offset = vec3<int16_t>{0, word(98), word(100)};
  result.camera_focus = vec3<int16_t>{0, word(102), word(104)};
  result.torso = {.sequence{word(26)}, .rate{word(28)}, .acceleration{word(30)}, .minimum{static_cast<int16_t>(-word(32))}, .maximum{word(32)}};
  result.elevation = {.sequence{word(34)}, .rate{word(36)}, .acceleration{word(38)}, .minimum{word(42)}, .maximum{word(40)}};
  auto const texture{word(148)};
  if(texture < 0 || texture > 3) file.fail("unknown texture group");
  result.textures = static_cast<herc_texture_group>(texture);
  for(size_t offset{46}; offset != 96 && source[offset] < 128; ++offset) result.detail_part_ids.push_back(source[offset]);
  for(size_t offset{86}; offset != 98 && source[offset]; ++offset) result.name.push_back(static_cast<char>(source[offset]));
  return result;
}

} // namespace earthsiege
