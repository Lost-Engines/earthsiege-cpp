#pragma once
#include "resources/mission_script.h"

namespace earthsiege {

// shared readers for payloads present in both source and compiled missions
auto read_routes_record(binary_reader &file)->mission_route;
auto read_areas_record(binary_reader &file)->trigger_area;
auto read_triggers_record(binary_reader &file)->mission_trigger;
auto read_hercs_record(binary_reader &file)->herc_spawn;
auto read_flyers_record(binary_reader &file)->flyer_spawn;
auto read_bases_record(binary_reader &file)->base_spawn;
auto read_groups_record(binary_reader &file)->group_spawn;

} // namespace earthsiege
