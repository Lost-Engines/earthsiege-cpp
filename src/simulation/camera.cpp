#include "camera.h"
#include <algorithm>
#include <stdexcept>
#include "simulation/fixed_math.h"
#include "simulation/model_math.h"

namespace earthsiege {
namespace {

int16_t constexpr pitch_limit{0x3000};
int32_t constexpr minimum_camera_height{100};
int16_t constexpr minimum_orbit_distance{2500};
int16_t constexpr maximum_orbit_distance{30000};

void turn_camera(vec3<uint16_t> &angles, vec3<int16_t> &rates) {
  for(unsigned int axis{0}; axis != 3; ++axis) angles[axis] = static_cast<uint16_t>(angles[axis] + rates[axis]);
  auto const pitch{wrap_word(angles.x)};
  if(pitch > pitch_limit || pitch < -pitch_limit) {
    angles.x = static_cast<uint16_t>(std::clamp<int16_t>(pitch, -pitch_limit, pitch_limit));
    rates.x = 0;
  }
}

vec3<int32_t> add_position(vec3<int32_t> const &a, vec3<int32_t> const &b) {
  return vec3<int32_t>{wrap_long(static_cast<int64_t>(a.x) + b.x), wrap_long(static_cast<int64_t>(a.y) + b.y), wrap_long(static_cast<int64_t>(a.z) + b.z)};
}

} // anonymous namespace

void steer_camera(camera_state &camera, bool const zoom, int16_t const tilt, int16_t const turn) {
  if(camera.mode == camera_mode::attached) return;
  auto const approach{[](int16_t &value, int16_t const desired, int16_t const step){
    if(value < desired) value = std::min(desired, wrap_word(value + step));
    else if(value > desired) value = std::max(desired, wrap_word(value - step));
  }};
  int const sign{camera.mode == camera_mode::free ? -1 : 1};
  approach(camera.angular_rates.z, wrap_word(sign * turn * 8), 192);
  approach(camera.movement, wrap_word(zoom ? sign * tilt * 4 : 0), 80);
  approach(camera.angular_rates.x, wrap_word(zoom ? 0 : tilt * 8), 192);
}

void update_camera(camera_state &camera, camera_target const &target) {
  switch(camera.mode) {
  case camera_mode::free:
    turn_camera(camera.angles, camera.angular_rates);
    camera.position = add_position(camera.position, rotate_vector(vec3<int32_t>{0, camera.movement, 0}, rotation_transform(camera.angles)));
    camera.position.z = std::max(minimum_camera_height, camera.position.z);
    return;
  case camera_mode::orbit:
    {
      auto const old_position{camera.position};
      auto const old_angles{camera.angles};
      auto const old_orbit{camera.orbit_angles};
      camera.orbit_distance = std::clamp<int16_t>(wrap_word(camera.orbit_distance + camera.movement), minimum_orbit_distance, maximum_orbit_distance);
      turn_camera(camera.orbit_angles, camera.angular_rates);
      camera.angles = camera.orbit_angles;
      camera.angles.z = static_cast<uint16_t>(camera.angles.z + target.angles.z);
      auto const centre{transform_point(camera.focus_offset, target.placement)};
      auto const locate{[&]{
        return add_position(centre, rotate_vector(vec3<int32_t>{0, -camera.orbit_distance, 0}, rotation_transform(camera.angles)));
      }};
      camera.position = locate();
      if(camera.position.z < minimum_camera_height) {
        if(camera.movement <= 0) {
          camera.position = old_position;
          camera.angles = old_angles;
          camera.orbit_angles = old_orbit;
        } else {
          auto const height{wrap_long(static_cast<int64_t>(centre.z) - minimum_camera_height)};
          auto const sine{wrap_word(wrap_long(static_cast<int64_t>(height) * model_matrix_one) / camera.orbit_distance)};
          camera.orbit_angles.x = camera.angles.x = model_arcsine(sine);
          camera.position = locate();
        }
      }
      return;
    }
  case camera_mode::attached:
    if(target.attachment) {
      auto const placement{compose_transforms(*target.attachment, target.placement)};
      camera.position = transform_point(camera.attachment_offset, placement);
      camera.angles = transform_angles(placement);
      for(unsigned int axis{0}; axis != 3; ++axis) camera.angles[axis] = static_cast<uint16_t>(camera.angles[axis] + camera.look_offset[axis]);
    } else {
      camera.position = transform_point(camera.attachment_offset, target.placement);
      camera.angles = target.angles;
    }
    return;
  }
  throw std::invalid_argument{"unknown camera mode"};
}

} // namespace earthsiege
