#pragma once
#include "resources/mission_compiler.h"
#include "shell/new_career.h"
namespace earthsiege {
struct mission_options {
  int16_t unlimited{},invulnerable{},difficulty{2},environment{},time{};
};
struct prepared_mission { compiled_mission compiled;mission_counters counters{}; };
prepared_mission prepare_mission(bytes source,int16_t mode,int16_t node,int16_t step,
  mission_options const &options,mission_counters const &history,random_state &random);
struct single_mission_crew {career_state state;hangar roster;armory inventory;};
single_mission_crew generate_single_crew(resource_store const &resources,mission_script const &script,random_state &random);
/// Original GO 0x22a05. In PLAYER.MEC `slot` is a portrait identity, while in
/// hangar files the corresponding word is the storage slot number.
player_roster launch_player_roster(career_state const &crew,hangar &roster,bool campaign,
  int16_t player_identity,random_state &random);
} // namespace earthsiege
