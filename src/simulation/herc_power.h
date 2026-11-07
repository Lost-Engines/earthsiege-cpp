#pragma once
#include <functional>
#include "simulation/component_damage.h"
#include "simulation/frame_time.h"
#include "simulation/shield_power.h"

namespace earthsiege {

inline int16_t constexpr herc_power_capacity{10000};
inline int16_t constexpr herc_power_reserve{1500};
inline int16_t constexpr reactor_generation{15};

struct herc_power {
  int16_t reserve{herc_power_capacity};
  shield_power shields;
};

using weapon_power_allocator = std::function<int16_t(int16_t)>;
/// Ratio-scaled reactor supply, then weapon allocation, then shield charging and reserve clamp
void update_herc_power(herc_power &state, damage_consequences const &damage, frame_ratio ratio, weapon_power_allocator const &allocate, int16_t shield_charge = shield_charge_per_update);

} // namespace earthsiege
