#include "aim_control.h"
#include <algorithm>
#include <cstdlib>
#include "simulation/fixed_math.h"
#include "simulation/frame_time.h"

namespace earthsiege {

void update_aim(aiming_state &state, aiming_profile const &profile, int16_t const input, int16_t const time_ratio, std::optional<int16_t> const target) {
  /// Original acceleration and averaged-rate integration use signed Q8 frame time
  auto const timed{[time_ratio](int32_t const value){ return scale_frame_time(wrap_word(value), time_ratio); }};
  auto const old_angle{state.angle};
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
  // Expansion 0x168c0/0x16b04 stop a tracking servo when it crosses its target.
  if(target) {
    auto const before{wrap_word(old_angle - *target)}, after{wrap_word(state.angle - *target)};
    if((before < 0 && after > 0) || (before > 0 && after < 0)) {state.angle = *target; state.rate = 0;}
  }
}

} // namespace earthsiege
