#include "frame_clock.h"
#include "fixed_math.h"
namespace earthsiege {
void frame_clock::advance(unsigned ticks) {
  for(unsigned i{0}; i < ticks; ++i) {
    if(state_.countdown) state_.countdown = wrap_word(int(state_.countdown) - 1);
    if(state_.fast++ & 1) ++state_.slow;
  }
}
bool frame_clock::ready() const {
  return state_.countdown <= wrap_word(int(state_.maximum) - 2);
}
std::optional<frame_ratio> frame_clock::take() {
  if(!ready()) return {};
  return consume(wrap_word(int(state_.maximum) - state_.countdown));
}
std::optional<frame_ratio> frame_clock::take_world() {
  auto const elapsed{wrap_word(int(state_.maximum) - state_.countdown)};
  if(elapsed < state_.nominal) return {};
  // A long stall gets the same single bounded update as DOS. At ordinary
  // refresh rates carry spare ticks forward to the next world deadline.
  if(ratio_from_period(elapsed, state_.nominal) == maximum_frame_ratio) return consume(elapsed);
  return consume(state_.nominal);
}
frame_ratio frame_clock::consume(int16_t ticks) {
  state_.elapsed = ticks;
  state_.ratio = ratio_from_period(state_.elapsed, state_.nominal);
  state_.alternate_ratio = ratio_from_period(state_.elapsed, state_.alternate_nominal);
  state_.countdown = wrap_word(int(state_.countdown) + ticks);
  return state_.ratio;
}
} // namespace earthsiege
