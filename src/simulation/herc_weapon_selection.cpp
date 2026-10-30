#include "herc_weapon_selection.h"
#include <algorithm>
#include <stdexcept>
#include "simulation/fixed_math.h"
#include "simulation/projectile_flight.h"

namespace earthsiege {
std::optional<size_t> weapon_lock_channel(weapon_state const &weapon, weapon_catalogue const &definitions) {
  if(!weapon.projectile) return std::nullopt;
  auto const &projectile{definitions.projectiles.at(*weapon.projectile)};
  if(projectile.kind != projectile_kind::rocket) return std::nullopt;
  if(projectile.subtype < 0 || projectile.subtype >= static_cast<int>(missile_lock_channels)) throw std::out_of_range{"missile lock channel"};
  return static_cast<size_t>(projectile.subtype);
}
bool weapon_excluded_from_combat(weapon_state const &weapon) {
  if(std::holds_alternative<ecm_weapon>(weapon.mechanism)) return true;
  auto const supply{weapon.ammunition()};
  return weapon.controls().disabled || (supply && supply->rounds == 0);
}

std::optional<size_t> select_herc_weapon(weapon_bank const &bank, weapon_catalogue const &definitions, herc_weapon_selection_state &state, combat_actor_kind const target_kind, int32_t const distance, int16_t const facing, random_state &random, int16_t &mission_event_delay, std::function<int16_t(int16_t)> const &target_shield, std::function<void()> const &unarmed_event) {
  // Original linear mapper rounds the descending 150..-100 threshold toward
  // its far endpoint. Its signed-word result is also retained after each winner.
  int16_t best{wrap_word(150 + (-250 * state.aim_tolerance - 1023) / 1024)};
  auto const shield{target_shield(facing >= -16384 && facing < 16384)};
  int16_t const variation{static_cast<int16_t>(target_kind == combat_actor_kind::herc ? 35 : 30)};
  bool unarmed{true};
  std::optional<size_t> selected;
  for(size_t i{0}; i != bank.slots().size(); ++i) {
    auto const &slot{bank.slots()[i]};
    if(!slot || weapon_excluded_from_combat(*slot)) continue;
    auto const &weapon{*slot};
    auto const &source{definitions.weapons.at(static_cast<size_t>(weapon.definition))};
    unarmed = false;
    auto const channel{weapon_lock_channel(weapon, definitions)};
    bool const in_range{weapon_in_range(source, distance)};
    if(channel == 0 && in_range && !state.detectable && state.ecm_remaining == 0) state.detectable = true;
    if((channel && *channel != rocket_manual_subtype && !state.locks[*channel]) || !in_range || !weapon.ready(source)) continue;
    if(!weapon.projectile) throw std::runtime_error{"unused grenade has no valid original projectile definition"};
    auto const &projectile{definitions.projectiles.at(*weapon.projectile)};
    int32_t value{shield == 0 ? projectile.armour_damage : shield < projectile.shield_damage ? projectile.armour_damage + 10000 : projectile.shield_damage};
    auto const first{random.bounded(variation)}, second{random.bounded(variation)};
    value = ((value * 100 + 512) >> 10) - ((1000 * source.shot_value_penalty + 512) >> 10) + first * second;
    if(value > best) { best = wrap_word(value); selected = i; }
  }
  if(unarmed && !state.unavailable) {
    state.unavailable = true;
    if(unarmed_event) unarmed_event();
    mission_event_delay = std::max<int16_t>(1000, mission_event_delay);
  }
  return selected;
}
} // namespace earthsiege
