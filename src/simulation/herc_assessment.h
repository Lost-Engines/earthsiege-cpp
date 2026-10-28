#pragma once
#include <array>
#include <functional>
#include "simulation/component_damage.h"
#include "simulation/target_selection.h"
#include "simulation/weapon_bank.h"
#include "resources/herc_profile.h"

namespace earthsiege {

int16_t herc_combat_rating(herc_profile const &profile, damage_definition const &definition, damage_state const &damage, weapon_bank const &bank, weapon_catalogue const &weapons);

/// The original ranks the target's present sections using the observer's damage,
/// and bounds weapon candidates by the observer's bank size. Both are preserved.
int16_t select_herc_aimed_component(component_damage const &observer, size_t observer_weapon_count, std::span<int16_t const> target_present, bool target_collapsed, random_state &random);

inline std::array<size_t, 6> constexpr retreat_components{0, 1, 4, 5, 6, 7};
struct herc_retreat_inputs {
  bool incapacitated{false};
  combat_actor_kind target_kind{combat_actor_kind::herc};
  bool target_can_retaliate{false};
  int16_t caution{0};
  std::array<int16_t, 6> internal_damage{};                                  // Q8 damage; a destroyed component is 256
  int16_t attackers{0};
  int16_t strength{0};
};
enum class herc_retreat_action { none, resume_orders, flee };
herc_retreat_action assess_herc_retreat(herc_retreat_inputs const &input, int16_t &aim_tolerance, std::function<int32_t()> const &attacker_strength);

} // namespace earthsiege
