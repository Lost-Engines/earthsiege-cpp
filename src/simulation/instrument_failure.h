#pragma once
#include "simulation/random_state.h"
#include <cstdint>

namespace earthsiege {
uint16_t instrument_damage_band(int32_t damage_fraction);
struct instrument_failure_state {
  uint16_t duration{};
  uint32_t window{}, next_flip{};
  bool failed{};
  uint8_t repaint{};
};
/// Separate UI RNG, nominal 60 Hz deadlines, and one event per update.
void schedule_instrument_failure(instrument_failure_state &state, uint32_t ticks, random_state &random);
/// Returns true when original sound 28 is requested. A zero damage band bypasses
/// failure processing; it does not silently repair the retained failure flag.
bool advance_instrument_failure(instrument_failure_state &state, uint16_t severity, uint32_t ticks, random_state &random);
} // namespace earthsiege
