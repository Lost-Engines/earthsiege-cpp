#include "herc_power.h"
#include <algorithm>
#include "simulation/fixed_math.h"

namespace earthsiege {

void update_herc_power(herc_power &state, damage_consequences const &damage, frame_ratio const ratio, weapon_power_allocator const &allocate) {
  auto generated{scale_frame_time(reactor_generation, ratio)};
  if(damage.reactor_heavy) generated = wrap_word((generated * 200 + 512) >> 10);
  else if(damage.reactor_light) generated = wrap_word((generated * 600 + 512) >> 10);
  auto const available{wrap_word(wrap_word(state.reserve + generated) - herc_power_reserve)};
  auto const remaining{recharge_shields(state.shields, allocate(available))};
  state.reserve = std::clamp<int16_t>(wrap_word(remaining + herc_power_reserve), 0, herc_power_capacity);
}

} // namespace earthsiege
