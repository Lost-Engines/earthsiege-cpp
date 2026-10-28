#pragma once
#include <array>
#include "simulation/fixed_math.h"
#include "simulation/herc_impact.h"

namespace earthsiege {

/// Original difficulty adjustment applies to damage fired by side zero only.
inline int16_t adjust_impact_damage(int16_t const amount, impact_conditions const &conditions) {
  if(conditions.attacker_side != 0) return amount;
  std::array<int16_t, 4> constexpr multipliers{2800, 2000, 1400, 1000};
  auto const multiplier{multipliers.at(conditions.difficulty_override ? 3 : static_cast<size_t>(conditions.difficulty))};
  return wrap_word((amount * multiplier + 512) >> 10);
}

} // namespace earthsiege
