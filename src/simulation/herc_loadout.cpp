#include "herc_loadout.h"
#include <algorithm>
#include <stdexcept>
#include <utility>
#include "simulation/fixed_math.h"

namespace earthsiege {

herc_loadout assemble_herc_loadout(herc_definition const &body, std::span<int16_t const> const equipment, weapon_catalogue const &weapons) {
  herc_loadout result{body.damage, body.collision};
  for(auto const &mount : body.mounts) {
    if(mount.equipment_slot < 0 || static_cast<size_t>(mount.equipment_slot) >= equipment.size()) throw std::out_of_range{"combat equipment slot"};
    auto const slot{static_cast<size_t>(mount.equipment_slot)};
    if(equipment[slot] == empty_weapon_slot) continue;
    auto const &weapon{weapons.weapons.at(static_cast<size_t>(equipment[slot]))};
    auto const section{weapon_section_first + slot};
    auto const internal{weapon_internal_first + slot};
    auto part{weapon.damage};
    part.parent = result.damage.components.at(section).parent;
    if(part.dependents.empty()) throw std::runtime_error{"weapon has no internal damage dependency"};
    part.dependents.front().component = static_cast<int16_t>(internal);
    result.damage.components[section] = std::move(part);
    result.damage.hit_points.at(internal) = weapon.internal_maximum;
    auto spheres{weapon.collision.spheres};
    auto const offset{weapon_mount_offset(mount, weapon)};
    for(auto &sphere : spheres) for(unsigned int axis{0}; axis != 3; ++axis) sphere.centre[axis] = wrap_word(sphere.centre[axis] + offset[axis]);
    // The original replaces the first pre-existing section with this equipment slot.
    bool replaced{false};
    for(auto &group : result.collision) {
      for(auto &candidate : group.sections) {
        if(candidate.section == static_cast<int16_t>(section)) {
          candidate.spheres = std::move(spheres);
          replaced = true;
          break;
        }
      }
      if(replaced) break;
    }
  }
  return result;
}

damage_state initial_herc_damage(herc_loadout const &loadout, size_t const cell_sequences) {
  damage_state result;
  result.internal.resize(loadout.damage.hit_points.size(), 0);
  result.armour.resize(loadout.damage.components.size(), 0);
  result.cells.resize(cell_sequences, 0);
  // Original memset(..., 1) fills each signed word with 0x0101; consumers test nonzero.
  for(auto const &component : loadout.damage.components) result.present.push_back(component.dependents.empty() ? 0 : 0x0101);
  return result;
}

} // namespace earthsiege
