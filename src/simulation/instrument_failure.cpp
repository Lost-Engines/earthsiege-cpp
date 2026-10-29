#include "instrument_failure.h"
#include "simulation/fixed_math.h"

namespace earthsiege {
uint16_t instrument_damage_band(int32_t const damage_fraction) {
  auto const percent{wrap_long((256-static_cast<int64_t>(damage_fraction))*100)>>8};
  return percent>=90 ? 0 : percent>=74 ? 1 : percent>=51 ? 2 : percent>=1 ? 3 : 4;
}
void schedule_instrument_failure(instrument_failure_state &state, uint32_t const ticks, random_state &random) {
  state.window=ticks+(static_cast<uint16_t>(random.next())%6+2)*60;
  state.next_flip=state.window;
  state.duration=static_cast<uint16_t>(static_cast<uint16_t>(random.next())%240+1);
}
bool advance_instrument_failure(instrument_failure_state &state, uint16_t const severity, uint32_t const ticks, random_state &random) {
  if(severity==0) return false;
  if(severity==3) {state.failed=true; return false;}
  if(state.window<ticks) {
    if(ticks<state.window+state.duration) {
      if(state.next_flip<ticks) {
        state.next_flip=ticks+static_cast<uint16_t>(random.next())%30+1;
        state.failed=!state.failed; state.repaint=2;
        return state.failed;
      }
    } else {
      schedule_instrument_failure(state,ticks,random);
      state.failed=severity>=2; state.repaint=2;
    }
  }
  return false;
}
} // namespace earthsiege
