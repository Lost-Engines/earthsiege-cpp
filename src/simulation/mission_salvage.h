#pragma once
#include "resources/mission_results.h"
#include "simulation/component_damage.h"

namespace earthsiege {
/// Returned percentages truncate the original Q8 damage fractions. The tenth
/// equipment word has no allocated original slot; it is explicitly 100 here.
herc_condition returned_condition(damage_definition const &definition,damage_state const &state);
struct salvaged_herc {int32_t score{};std::vector<std::pair<int16_t,int16_t>> weapons;};
struct salvage_weapon {int16_t slot{},definition{};};
salvaged_herc assess_salvage(damage_definition const &definition,damage_state const &state,
  std::span<salvage_weapon const> weapons,int16_t hull_value,bool excluded=false);
int32_t recovery_value(int32_t score,int16_t mission_awards);
} // namespace earthsiege
