#include "simulation/shield_power.h"
#include <algorithm>
#include "simulation/fixed_math.h"

namespace earthsiege {

void change_shield_balance(shield_power &state, bool const toward_front) {
  state.balance = std::clamp<int16_t>(wrap_word(state.balance + (toward_front ? shield_balance_step : -shield_balance_step)), 0, shield_balance_one);
}

int16_t absorb_blast(shield_power &state,bool front,int16_t amount){
  auto &shield{front?state.front:state.rear};shield=wrap_word(shield-wrap_word((1000*amount+512)>>10));
  if(shield>=0)return 0;
  auto remaining{wrap_word((-static_cast<int>(shield)*1024)/1000)};shield=0;return remaining;
}

int16_t recharge_shields(shield_power &state, int16_t const budget) {
  auto const amount{std::min({shield_charge_per_update, budget, wrap_word(state.capacity - wrap_word(state.front + state.rear))})};
  auto const total{wrap_word(state.front + state.rear + amount)};
  auto const desired{wrap_word((total * state.balance + shield_balance_one / 2) >> 10)};
  if(desired < state.front) state.front = std::max(desired, wrap_word(state.front - shield_transfer_per_update));
  else if(desired > state.front) state.front = std::min(desired, wrap_word(state.front + shield_transfer_per_update));
  state.rear = wrap_word(total - state.front);
  return wrap_word(budget - amount);
}

} // namespace earthsiege
