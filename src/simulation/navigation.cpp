#include "navigation.h"
#include <algorithm>
#include <array>
#include <cstdlib>
#include <stdexcept>
#include "simulation/fixed_math.h"
#include "simulation/model_math.h"

namespace earthsiege {
namespace {

vec3<int32_t> difference(vec3<int32_t> const &a, vec3<int32_t> const &b) {
  return vec3<int32_t>{wrap_long(static_cast<int64_t>(a.x) - b.x), wrap_long(static_cast<int64_t>(a.y) - b.y), wrap_long(static_cast<int64_t>(a.z) - b.z)};
}

int32_t horizontal_distance(vec3<int32_t> const &a, vec3<int32_t> const &b) {
  auto const delta{difference(a, b)};
  return fast_distance_2d(vec2<int32_t>{delta.x, delta.y});
}

} // namespace

movement_request follow_route(vec3<int32_t> const &position, uint16_t const heading, int16_t const requested_speed, mission_route const &route, std::span<vec3<int32_t> const> const points, int16_t &cursor) {
  int const next{cursor + 1};
  if(next < 0 || static_cast<size_t>(next) >= route.points.size()) return {};
  auto const point{route.points[static_cast<size_t>(next)]};
  if(point < 0 || static_cast<size_t>(point) >= points.size()) throw std::out_of_range{"route point"};
  auto const &destination{points[static_cast<size_t>(point)]};
  movement_request const result{.speed{requested_speed ? requested_speed : int16_t{170}}, .turn{static_cast<int16_t>(wrap_word(bearing_angle(position, destination) - heading) >> 4)}};
  int32_t constexpr arrival_distance{10000};
  if(horizontal_distance(position, destination) < arrival_distance) {
    cursor = static_cast<int16_t>(next);
    if(route.points.size() > 1 && static_cast<size_t>(cursor) == route.points.size() - 1 && route.points.front() == route.points.back()) cursor = 0;
  }
  return result;
}

movement_request follow_formation(vec3<int32_t> const &position, uint16_t const heading, vec3<int32_t> const &leader_position, uint16_t const leader_heading, int16_t const leader_speed, vec3<int32_t> const &offset) {
  auto const rotation{rotation_transform(vec3<uint16_t>{0, 0, leader_heading})};
  auto destination{rotate_vector(offset, rotation)};
  for(unsigned int axis{0}; axis != 3; ++axis) destination[axis] = wrap_long(static_cast<int64_t>(destination[axis]) + leader_position[axis]);
  auto const distance{horizontal_distance(position, destination)};
  auto const heading_error{wrap_word(heading - leader_heading)};
  int32_t constexpr formation_stop_distance{2000};
  int32_t constexpr formation_follow_distance{25000};
  int constexpr formation_alignment_limit{8192};
  if(distance < formation_stop_distance) return {.turn{wrap_word(-heading_error >> 4)}};
  if(distance <= formation_follow_distance && (leader_speed < -25 || leader_speed >= 25)) {
    if(std::min(std::abs(static_cast<int>(heading_error)), 32767) < formation_alignment_limit) {
      auto delta{difference(position, destination)};
      delta.z = 0;                                                             // original formation correction uses the horizontal matrix only
      auto const local{inverse_rotate_vector(delta, rotation)};
      return {.speed{wrap_word((-local.y >> 5) + leader_speed)}, .turn{wrap_word(-((heading_error >> 4) + (-local.x >> 5)))}};
    }
    return {.speed{-256}, .turn{static_cast<int16_t>(heading_error <= 0 ? 256 : -256)}};
  }
  return {.speed{static_cast<int16_t>(distance < formation_follow_distance ? distance >> 7 : 256)}, .turn{static_cast<int16_t>(wrap_word(bearing_angle(position, destination) - heading) >> 4)}};
}

void avoid_obstacles(movement_request &control, avoidance_state const &state, std::span<contact_actor const> const actors, avoidance_query const &query) {
  if((state.current_speed == 0 && control.speed == 0) || (control.speed < 0 && state.collision_countdown == 0)) return;
  if(control.flags & 1) return;                                                 // ramming bypasses both scenery probes and actor clearance
  int32_t constexpr clear_distance{22000};
  std::array<int32_t, 2> clearance{clear_distance, clear_distance};
  auto const rotation{rotation_transform(vec3<uint16_t>{0, 0, state.heading})};
  for(size_t side{0}; side != clearance.size(); ++side) {
    int const direction{side == 0 ? -1 : 1};
    auto start{rotate_vector(vec3<int32_t>{1500 * direction, 0, 0}, rotation)};
    auto end{rotate_vector(vec3<int32_t>{10000 * direction, 20000, 0}, rotation)};
    for(unsigned int axis{0}; axis != 3; ++axis) {
      start[axis] = wrap_long(static_cast<int64_t>(start[axis]) + state.position[axis]);
      end[axis] = wrap_long(static_cast<int64_t>(end[axis]) + state.position[axis]);
    }
    for(auto const kind : {avoidance_probe::scenery, avoidance_probe::ground_objects}) if(auto const distance{query(kind, start, end)}) clearance[side] = std::min(clearance[side], *distance);
  }
  for(auto const &actor : actors) {
    if(actor.waiting || actor.radius == 0) continue;
    auto const distance{wrap_long((static_cast<int64_t>(fast_distance(difference(actor.position, state.position))) * 2000 + 512) >> 10)};
    if(distance >= clearance[0] && distance >= clearance[1]) continue;
    auto const angle{wrap_word(bearing_angle(state.position, actor.position) - state.heading)};
    if(std::min(std::abs(static_cast<int>(angle)), 32767) >= 8192) continue;
    auto &side{clearance[angle < 0 ? 1 : 0]};
    side = std::min(side, distance);
  }
  if(clearance[0] >= clear_distance && clearance[1] >= clear_distance) return;
  if(state.forward_collision && clearance[0] < clear_distance && clearance[1] < clear_distance) clearance[0] = wrap_long(static_cast<int64_t>(clearance[0]) + state.collision_bias);
  auto const distance{std::min(clearance[0], clearance[1])};
  auto const correction{wrap_word(wrap_long(static_cast<int64_t>(clear_distance - distance) * 2000 / clear_distance))};
  control.turn = wrap_word(control.turn + (clearance[0] <= clearance[1] ? -correction : correction));
  if(distance < 0) control.speed = 0;
}

} // namespace earthsiege
