#pragma once
#include <cstdint>
#include <span>
#include <vector>
#include "vectorstorm/vector/vector2.h"
#include "vectorstorm/vector/vector3.h"

namespace earthsiege {

struct camera_projection {
  unsigned int perspective_shift{8};                                           // zero selects parallel projection
  int32_t near_distance{256};
  int32_t orthographic_divisor{256};
  vec2<int32_t> centre{160, 100};
};

enum class projection_source {
  model,
  terrain,
};

struct camera_vertex {
  vec3<int32_t> position;
  vec2<int32_t> texture;
  uint16_t shade{0};
};

struct projected_vertex {
  vec2<int32_t> position;
  vec2<int32_t> texture;
  uint16_t shade{0};
};

bool inside_near_plane(vec3<int32_t> const &point, camera_projection const &view, projection_source source);
vec2<int32_t> project_point(vec3<int32_t> const &point, camera_projection const &view, projection_source source);
std::vector<projected_vertex> project_polygon(std::span<camera_vertex const> vertices, camera_projection const &view, projection_source source);

} // namespace earthsiege
