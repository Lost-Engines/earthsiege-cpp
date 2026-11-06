#pragma once
#include <cstdint>

namespace earthsiege {

inline int16_t constexpr shield_balance_one{1024};
inline int16_t constexpr shield_balance_step{102};
inline int16_t constexpr shield_charge_per_update{4};
inline int16_t constexpr shield_transfer_per_update{65};

struct shield_power {
  int16_t front{0};
  int16_t rear{0};
  int16_t balance{shield_balance_one / 2};
  int16_t capacity{0};
};

void change_shield_balance(shield_power &state, bool toward_front);
/// Expansion radial hits use Q10 damage against shield units scaled by 1000.
int16_t absorb_blast(shield_power &state,bool front,int16_t amount);
int16_t recharge_shields(shield_power &state, int16_t budget);

} // namespace earthsiege
