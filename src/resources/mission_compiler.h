#pragma once
#include "edition.h"
#include <cstdint>
#include <span>
#include <vector>
#include "resources/mission_conditions.h"
#include "resources/mission_script.h"

namespace earthsiege {

class random_state;

struct compiled_mission {
  mission_script script;
  std::vector<mission_condition> conditions;
};

auto compile_mission(bytes source, std::span<int16_t const> campaign_flags, random_state &random, int16_t campaign_mode, game_edition edition = game_edition::original)->compiled_mission;

} // namespace earthsiege
