#include "herc_aiming.h"
#include <algorithm>
#include <cstdlib>
#include "simulation/fixed_math.h"
#include "simulation/model_math.h"

namespace earthsiege {
namespace {
vec3<int16_t> aim_angles(vec3<int32_t> const &delta) {
  return {wrap_word(direction_angle(fast_distance_2d({delta.x, delta.y}), delta.z)), 0,
          wrap_word(direction_angle(delta.x, delta.y) - angle_quarter_turn)};
}
int16_t aim_response(int16_t const angle) {
  return static_cast<int16_t>(std::clamp((250 * angle + 512) >> 10, -256, 256));
}
vec3<int32_t> inverse_point(vec3<int32_t> const &point, model_transform const &matrix) {
  auto const inverse{inverse_transform(matrix)};
  auto result{rotate_vector(point, inverse)};
  for(unsigned int axis{0}; axis != 3; ++axis) result[axis] = wrap_long(static_cast<int64_t>(result[axis]) + inverse.translation[axis]);
  return result;
}
} // anonymous namespace

herc_aim_selection select_herc_aim_point(combat_actor_kind const target_kind, vec3<int32_t> const &origin, uint16_t const heading, int16_t const torso_limit, vec3<int32_t> const &target_position, int16_t const component) {
  if(target_kind != combat_actor_kind::herc) return {herc_aim_mode::target_centre};
  auto const error{wrap_word(bearing_angle(origin, target_position) - heading)};
  if(static_cast<uint16_t>(error + torso_limit) >= static_cast<uint16_t>(torso_limit * 2)) return {};
  vec3<int32_t> delta;
  for(unsigned int axis{0}; axis != 3; ++axis) delta[axis] = wrap_long(static_cast<int64_t>(target_position[axis]) - origin[axis]);
  auto const distance{fast_distance(delta)};
  if(component >= 0 && distance >= 5000 && distance < 20000) return {herc_aim_mode::target_component, component};
  return {herc_aim_mode::target_centre};
}

herc_aim_request aim_herc_at(vec3<int32_t> const &point, model_transform const &placement, model_transform const &pivot, int16_t const eye_height) {
  auto local{inverse_point(inverse_point(point, placement), pivot)};
  local.z = wrap_long(static_cast<int64_t>(local.z) - eye_height);
  auto const errors{aim_angles(local)};
  vec2<int16_t> input{static_cast<int16_t>(-aim_response(errors.z)), aim_response(errors.x)};
  for(unsigned int axis{0}; axis != 2; ++axis) input[axis] = static_cast<int16_t>((input[axis] * std::abs(input[axis]) + 128) >> 8);
  return {vec3<int16_t>{errors}, vec2<int16_t>{input}, fast_distance(local)};
}

herc_aim_request centre_herc_aim(int16_t const torso, int16_t const elevation, int32_t const convergence_distance) {
  // Recentering deliberately omits the squared response used for target tracking.
  return {{}, {static_cast<int16_t>(-aim_response(torso)), static_cast<int16_t>(-aim_response(elevation))}, convergence_distance};
}

vec3<int16_t> herc_weapon_convergence(vec3<int16_t> const &mount_offset, int16_t const eye_height, int32_t const distance) {
  if(distance == 0) return {};
  return aim_angles({-static_cast<int32_t>(mount_offset.x), wrap_long(static_cast<int64_t>(distance) - mount_offset.y), static_cast<int32_t>(eye_height) - mount_offset.z});
}
} // namespace earthsiege
