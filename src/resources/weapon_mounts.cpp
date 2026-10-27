#include "weapon_mounts.h"
#include <algorithm>

namespace earthsiege {

bool weapon_mount::external() const {
  return orientation < 4;
}

std::vector<weapon_mount> read_weapon_mounts(bytes const source) {
  binary_reader file{source, "weapon mounts"};
  auto const count{file.integer<uint16_t>()};
  std::vector<weapon_mount> result;
  result.reserve(count);
  for(unsigned int index{0}; index != count; ++index) {
    weapon_mount mount;
    auto const record{file.take(mount.source.size())};
    std::ranges::copy(record, mount.source.begin());
    binary_reader fields{record};
    mount.model_part = fields.integer<int16_t>();
    fields.seek(6);
    mount.orientation = fields.integer<uint8_t>();
    fields.seek(16);
    mount.position = vec3<int16_t>{fields.integer<int16_t>(), fields.integer<int16_t>(), fields.integer<int16_t>()};
    fields.seek(23);
    mount.equipment_slot = fields.integer<int8_t>();
    result.push_back(mount);
  }
  file.require_end();
  return result;
}

} // namespace earthsiege
