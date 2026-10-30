#pragma once
#include <array>
#include <functional>
#include "simulation/target_selection.h"
#include "simulation/weapon_bank.h"

namespace earthsiege {
inline size_t constexpr missile_lock_channels{5};

/// A non-missile has no seeker channel and does not require a missile lock.
std::optional<size_t> weapon_lock_channel(weapon_state const &weapon, weapon_catalogue const &definitions);
bool weapon_excluded_from_combat(weapon_state const &weapon);

struct herc_weapon_selection_state {
  std::array<bool, missile_lock_channels> locks{};
  int16_t aim_tolerance{0};
  int16_t ecm_remaining{0};
  bool detectable{false};
  bool unavailable{false};
};

std::optional<size_t> select_herc_weapon(weapon_bank const &bank, weapon_catalogue const &definitions, herc_weapon_selection_state &state, combat_actor_kind target_kind, int32_t distance, int16_t facing, random_state &random, int16_t &mission_event_delay, std::function<int16_t(int16_t)> const &target_shield, std::function<void()> const &unarmed_event = {});
} // namespace earthsiege
