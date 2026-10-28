#pragma once
#include <span>
#include "resources/herc_definition.h"
#include "simulation/component_damage.h"

namespace earthsiege {

inline size_t constexpr herc_internal_slots{21};
inline size_t constexpr weapon_section_first{19};
inline size_t constexpr weapon_internal_first{12};

struct herc_loadout {
  damage_definition damage;
  std::vector<collision_part> collision;
};

herc_loadout assemble_herc_loadout(herc_definition const &body, std::span<int16_t const> equipment, weapon_catalogue const &weapons);
damage_state initial_herc_damage(herc_loadout const &loadout, size_t cell_sequences);

} // namespace earthsiege
