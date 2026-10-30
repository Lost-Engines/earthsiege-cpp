#pragma once
#include <optional>
#include "resources/model.h"

namespace earthsiege {

enum class camera_mode : int16_t {
  free,
  orbit,
  attached,
};

struct camera_state {
  vec3<int32_t> position;
  vec3<uint16_t> angles;
  int16_t orbit_distance{0};
  int16_t movement{0};
  vec3<uint16_t> orbit_angles;
  vec3<int16_t> angular_rates;
  camera_mode mode{camera_mode::attached};
  vec3<int16_t> focus_offset;
  vec3<int16_t> attachment_offset;
  vec3<int16_t> look_offset;
};

struct camera_target {
  vec3<uint16_t> angles;
  model_transform placement;
  std::optional<model_transform> attachment;
};

void steer_camera(camera_state &camera, bool zoom, int16_t tilt, int16_t turn);
void update_camera(camera_state &camera, camera_target const &target);

} // namespace earthsiege
