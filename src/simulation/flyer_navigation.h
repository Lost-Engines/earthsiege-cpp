#pragma once
#include "simulation/flyer_motion.h"

namespace earthsiege {
void follow_flyer_route(flyer_motion &flight, flyer_profile const &profile, mission_route const &route, std::span<vec3<int32_t> const> points, int16_t &cursor, int16_t &requested_speed);
void follow_flyer_formation(flyer_motion &flight, flyer_profile const &profile, flyer_motion const &leader, vec3<int32_t> const &formation_offset, int16_t &requested_speed);
} // namespace earthsiege
