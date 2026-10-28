#include "flyer_navigation.h"
#include <algorithm>
#include <stdexcept>
#include "simulation/fixed_math.h"
#include "simulation/model_math.h"

namespace earthsiege {
namespace {
vec3<int32_t> difference(vec3<int32_t> const &a, vec3<int32_t> const &b) {
  return {wrap_long(static_cast<int64_t>(a.x)-b.x),wrap_long(static_cast<int64_t>(a.y)-b.y),wrap_long(static_cast<int64_t>(a.z)-b.z)};
}
void add(vec3<int32_t> &to, vec3<int32_t> const &offset) {
  for(unsigned int axis{0}; axis != 3; ++axis) to[axis] = wrap_long(static_cast<int64_t>(to[axis]) + offset[axis]);
}
}
void follow_flyer_route(flyer_motion &flight, flyer_profile const &profile, mission_route const &route, std::span<vec3<int32_t> const> const points, int16_t &cursor, int16_t &requested_speed) {
  if(cursor < 0 || static_cast<size_t>(cursor) >= route.points.size()) throw std::out_of_range{"flyer route cursor"};
  auto const next{static_cast<size_t>(cursor) + 1};
  auto const index{route.points[next < route.points.size() ? next : static_cast<size_t>(cursor)]};
  if(index < 0 || static_cast<size_t>(index) >= points.size()) throw std::out_of_range{"flyer route point"};
  auto const &destination{points[static_cast<size_t>(index)]};
  if(next < route.points.size()) {
    auto const delta{difference(destination,flight.position)};
    if(fast_distance_2d({delta.x,delta.y}) < 15000) {
      cursor = static_cast<int16_t>(next);
      if(route.points.size() > 1 && next == route.points.size()-1 && route.points.front() == route.points.back()) cursor = 0;
    }
  }
  auto const error{wrap_word(bearing_angle(flight.position,destination) - flight.angles.z)};
  turn_flyer(flight,profile,wrap_word((error * 800 + 512) >> 10));
  requested_speed = 180;
  hold_flyer_altitude(flight,profile,10000);
}
void follow_flyer_formation(flyer_motion &flight, flyer_profile const &profile, flyer_motion const &leader, vec3<int32_t> const &formation_offset, int16_t &requested_speed) {
  auto destination{leader.position};
  add(destination,rotate_vector({0,8192,0},rotation_transform({0,0,leader.angles.z})));
  auto offset{rotate_vector({formation_offset.x,formation_offset.y,0},flyer_placement(leader))};
  offset.z = formation_offset.z;                                            // formation uses the leader's horizontal matrix block, with unrotated altitude
  add(destination,offset);
  auto const distance{fast_distance(difference(destination,flight.position))};
  auto bearing{wrap_word(bearing_angle(flight.position,destination) - flight.angles.z)};
  auto const alignment{wrap_word(flight.angles.z - leader.angles.z)};
  if(alignment >= -16384 && alignment < 16384 && (bearing < -16384 || bearing >= 16384) && distance < 10000) bearing = wrap_word(-32768 - bearing);
  turn_flyer(flight,profile,bearing,flyer_leader_turn{leader.angles.z,leader.angular_step.z});
  hold_flyer_altitude(flight,profile,10000);
  if(distance > 15000) requested_speed = 256;
  else {
    auto const cosine{model_cosine(static_cast<uint16_t>(bearing + flight.angles.z - leader.angles.z))};
    auto const projected{wrap_word((wrap_word(distance) * cosine + 8192) >> 14)};
    auto const correction{wrap_word(projected - 8192) >> 1};
    requested_speed = std::clamp<int16_t>(wrap_word(requested_speed + (leader.velocity.y - flight.velocity.y) * 8 + correction),140,256);
  }
}
} // namespace earthsiege
