#pragma once
#include "resources/player_roster.h"
#include <utility>
namespace earthsiege {
struct returned_herc {herc_condition condition;std::array<int16_t,3> statistics{};};
struct mission_results {
  int16_t grade{};
  int32_t recovery{};
  std::vector<std::pair<int16_t,int16_t>> salvage;
  std::vector<returned_herc> crew;
};
mission_results read_mission_results(bytes source);
std::vector<uint8_t> write_mission_results(mission_results const &results);
} // namespace earthsiege
