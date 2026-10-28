#include "herc_assessment.h"
#include <stdexcept>
#include "simulation/fixed_math.h"
#include "simulation/herc_loadout.h"

namespace earthsiege {
namespace {
int16_t fraction(int32_t amount, int32_t capacity) {
  if(capacity == 0) throw std::domain_error{"zero combat assessment capacity"};
  return wrap_word(wrap_long(static_cast<int64_t>(amount) * 256) / capacity);
}
int32_t surviving_rating(int16_t damage, int16_t rating) {
  return wrap_long(((256 - static_cast<int64_t>(damage)) * rating + 128) >> 8);
}
}
int16_t herc_combat_rating(herc_profile const &profile, damage_definition const &definition, damage_state const &damage, weapon_bank const &bank, weapon_catalogue const &weapons) {
  int32_t result{profile.base_combat_rating};
  // The original metric array is equipment-ordinal indexed, even though the
  // bank iterates mount order. Availability ignores disabled/refire/charge state.
  for(size_t i{0}; i != bank.slots().size(); ++i) {
    auto const &weapon{bank.slots()[i]};
    if(!weapon) continue;
    auto const section{weapon_section_first + i}, component{weapon_internal_first + i};
    int32_t const armour{definition.components.at(section).armour}, internal{definition.hit_points.at(component)};
    auto const grade{fraction((damage.armour.at(section) < 0 ? armour : damage.armour[section]) + (damage.internal.at(component) < 0 ? internal : damage.internal[component]), armour + internal)};
    if(grade < 0) continue;
    auto const &source{weapons.weapons.at(static_cast<size_t>(weapon->definition))};
    if(auto const supply{weapon->ammunition()}; supply && supply->rounds < (source.magazine >> 3)) continue;
    result = wrap_long(static_cast<int64_t>(result) + surviving_rating(grade, source.combat_rating));
  }
  for(size_t i{0}; i != weapon_section_first; ++i) {
    auto const capacity{definition.components.at(i).armour};
    auto const grade{damage.armour.at(i) < 0 ? int16_t{256} : fraction(damage.armour[i], capacity)};
    if(grade >= 0) result = wrap_long(static_cast<int64_t>(result) + surviving_rating(grade, capacity));
  }
  for(size_t i{0}; i != profile.internal_damage_penalties.size(); ++i) {
    auto const capacity{definition.hit_points.at(i)};
    auto const grade{capacity == 0 ? int16_t{0} : damage.internal.at(i) < 0 ? int16_t{256} : fraction(damage.internal[i], capacity)};
    if(grade > 180) result = wrap_long(static_cast<int64_t>(result) - profile.internal_damage_penalties[i]);
  }
  return wrap_word(result >> 4);
}

int16_t select_herc_aimed_component(component_damage const &observer, size_t const observer_weapon_count, std::span<int16_t const> const target_present, bool const target_collapsed, random_state &random) {
  auto const pick{random.next() & 127};                                    // consumed even for a collapsed target
  size_t const first{target_collapsed ? 0U : pick < 40 ? 7U : pick < 80 ? 19U : 0U};
  size_t const end{target_collapsed ? 1U : pick < 40 ? 19U : pick < 80 ? 19U + observer_weapon_count : 7U};
  if(end > target_present.size()) throw std::out_of_range{"combat component selection exceeds target sections"};
  int16_t best{0}, selected{-1};
  for(size_t i{first}; i != end; ++i) {
    if(target_present[i] == 0) continue;
    auto const grade{observer.fraction(i)};
    auto const score{wrap_word(grade + (random.next() & 63))};
    if(best < score) { best = score; selected = static_cast<int16_t>(i); }
  }
  return selected;
}

herc_retreat_action assess_herc_retreat(herc_retreat_inputs const &input, int16_t &aim_tolerance, std::function<int32_t()> const &attacker_strength) {
  if(input.incapacitated) return input.target_kind == combat_actor_kind::base && !input.target_can_retaliate ? herc_retreat_action::resume_orders : herc_retreat_action::flee;
  auto caution{input.caution};
  std::array<int16_t, 3> constexpr thresholds{60, 120, 180}, penalties{5, 25, 50};
  for(size_t level{0}; level != thresholds.size(); ++level) {
    bool affected{false};
    for(auto const damage : input.internal_damage) if(damage > thresholds[level]) { caution = wrap_word(caution + penalties[level]); affected = true; }
    if(!affected) break;
  }
  bool flee{false};
  if(caution >= 71) {
    aim_tolerance = 1000;
    flee = input.attackers != 0;
  } else if(caution >= 51) {
    aim_tolerance = 600;
    flee = input.attackers >= 3 || (input.attackers != 0 && attacker_strength() > input.strength);
  } else if(caution > 30) {
    aim_tolerance = 300;
    flee = input.attackers > 2 && attacker_strength() / input.attackers > input.strength;
  }
  return flee ? herc_retreat_action::flee : herc_retreat_action::none;
}
} // namespace earthsiege
