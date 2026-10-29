#pragma once
#include "shell/hangar_service.h"
#include "resources/mission_results.h"
#include "simulation/random_state.h"
namespace earthsiege {
struct campaign_debrief {
  int16_t deaths{};
  std::array<int16_t,3> statistics{};
};
int16_t pilot_rank(int16_t total);
void advance_technology(technology_state &technology,int16_t missions,int16_t kind,int16_t grade);
/// GO 0x38eb5 after loading the career/hangar; leaves the post-return state ready
/// for the dispatcher's presentation, repair preference and history save.
campaign_debrief return_from_mission(career_record &career,hangar &roster,mission_results const &result,
  std::array<int16_t,100> const &counters,service_prices const &prices,random_state &random);
} // namespace earthsiege
