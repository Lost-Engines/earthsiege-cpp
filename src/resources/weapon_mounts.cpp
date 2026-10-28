#include "weapon_mounts.h"
#include <algorithm>
#include "resources/combat.h"
#include "simulation/fixed_math.h"

namespace earthsiege {

bool weapon_mount::external() const {
  return orientation < 4;
}

vec3<int16_t> weapon_mount_offset(weapon_mount const &mount, weapon_definition const &definition) {
  vec3<int16_t> offset{mount.position};
  if(mount.orientation == 0) offset.z = wrap_word(offset.z + definition.side_muzzle_offset.z);
  else if(mount.orientation == 1) offset.z = wrap_word(offset.z - definition.side_muzzle_offset.z);
  else if(mount.orientation == 2) offset.x = wrap_word(offset.x - definition.side_muzzle_offset.x);
  else if(mount.orientation == 3) offset.x = wrap_word(offset.x + definition.side_muzzle_offset.x);
  return offset;
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
    mount.pitch_sequence = fields.integer<int16_t>();
    mount.yaw_sequence = fields.integer<int16_t>();
    mount.orientation = fields.integer<uint8_t>();
    mount.control_number = fields.integer<int8_t>();
    fields.seek(16);
    mount.position = vec3<int16_t>{fields.integer<int16_t>(), fields.integer<int16_t>(), fields.integer<int16_t>()};
    mount.paired_slot_offset = fields.integer<int8_t>();
    mount.equipment_slot = fields.integer<int8_t>();
    result.push_back(mount);
  }
  file.require_end();
  return result;
}

} // namespace earthsiege
