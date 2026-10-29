#pragma once
#include "resources/career.h"

namespace earthsiege {
struct chassis_prices {
  int32_t purchase{};
  int16_t construction_missions{};
  std::array<int16_t,6> armour{};
  std::array<int16_t,10> internal{};
};
struct service_prices {
  std::array<chassis_prices,8> chassis;
  std::array<int16_t,18> weapons;
};
service_prices read_service_prices(bytes damage_table);
int16_t herc_average_condition(player_herc const &herc);
int16_t reserve_construction(hangar &roster);
bool construct_herc(hangar &,career_state &,int16_t chassis,size_t mounts,service_prices const &);
bool scrap_selected(hangar &,career_state &,armory &,service_prices const &);
int16_t condition_tier(int16_t condition);
int32_t repair_quote(player_herc const &herc,int16_t target,service_prices const &prices);
struct repair_result { bool applied{};int32_t cost{}; };
repair_result repair_herc(player_herc &herc,int32_t budget,service_prices const &prices);
void apply_repair_policy(career_state &state,hangar &roster,service_prices const &prices);
void advance_construction(hangar &roster,service_prices const &prices);
/// GO 0x23883's destroyed-weapon cleanup when rebuilding a shell HERC.
void discard_destroyed_equipment(player_herc &herc);
void normalize_hangar_equipment(player_herc &herc);
/// Credits recovered from chassis components; mounted weapons handled separately.
int32_t chassis_recovery(player_herc const &herc,service_prices const &prices);
} // namespace earthsiege
