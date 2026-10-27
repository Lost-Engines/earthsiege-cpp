#include "aim_control.h"
#include <algorithm>
#include <cstdlib>
#include "simulation/fixed_math.h"

namespace earthsiege {

void update_aim(aiming_state &state, aiming_profile const &profile, int16_t const input, int16_t const frame_ratio) {
  /// Original acceleration and averaged-rate integration use signed Q8 frame time
  auto const timed{[frame_ratio](int32_t const value){ return std::max<int16_t>(-32767, wrap_word(wrap_long((static_cast<int64_t>(value) * frame_ratio + 128) >> 8))); }};
  auto const desired{wrap_word((static_cast<int32_t>(profile.rate) * input + 128) >> 8)};
  auto previous{state.rate};
  auto const magnitude{[](int16_t const value){ return std::min(32767, std::abs(static_cast<int>(value))); }};
  if(magnitude(previous) < magnitude(desired)) {
    auto const step{timed(profile.acceleration)};
    if(desired < state.rate) state.rate = std::max(desired, wrap_word(state.rate - step));
    else if(desired > state.rate) state.rate = std::min(desired, wrap_word(state.rate + step));
  } else previous = state.rate = desired;
  auto const movement{timed((static_cast<int32_t>(previous) + state.rate) >> 1)};
  state.angle = std::clamp(wrap_word(state.angle + movement), profile.minimum, profile.maximum);
}

} // namespace earthsiege
