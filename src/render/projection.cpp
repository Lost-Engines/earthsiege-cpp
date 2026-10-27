#include "projection.h"
#include <limits>
#include <stdexcept>
#include "simulation/fixed_math.h"

namespace earthsiege {

namespace {

int32_t divide(int32_t const numerator, int32_t const denominator) {
  if(denominator == 0 || (numerator == std::numeric_limits<int32_t>::min() && denominator == -1)) throw std::domain_error{"projection division outside native range"};
  return numerator / denominator;
}

int32_t interpolate(int32_t const from, int32_t const to, int32_t const numerator, int32_t const denominator) {
  auto const difference{wrap_long(static_cast<int64_t>(to) - from)};
  auto const product{wrap_long(static_cast<int64_t>(difference) * numerator)};
  return wrap_long(static_cast<int64_t>(from) + divide(product, denominator));
}

} // anonymous namespace

bool inside_near_plane(vec3<int32_t> const &point, camera_projection const &view, projection_source const source) {
  if(view.perspective_shift == 0) return true;
  return source == projection_source::model ? point.y > view.near_distance : point.y >= view.near_distance;
}

vec2<int32_t> project_point(vec3<int32_t> const &point, camera_projection const &view, projection_source const source) {
  /// Original coordinates use Y forward and Z up, with signed wrapping shifts
  auto const shift{view.perspective_shift ? view.perspective_shift & 31 : 8};
  auto const denominator{view.perspective_shift ? point.y : view.orthographic_divisor};
  auto const horizontal{wrap_long(static_cast<int64_t>(point.x) * (int64_t{1} << shift))};
  auto const vertical{wrap_long(static_cast<int64_t>(point.z) * (int64_t{1} << shift))};
  auto const x{wrap_long(static_cast<int64_t>(view.centre.x) + divide(horizontal, denominator))};
  // Model perspective negates after division; the other paths negate before it.
  auto const y{view.perspective_shift && source == projection_source::model
    ? wrap_long(static_cast<int64_t>(view.centre.y) - divide(vertical, denominator))
    : wrap_long(static_cast<int64_t>(view.centre.y) + divide(wrap_long(-static_cast<int64_t>(vertical)), denominator))};
  return {x, y};
}

std::vector<projected_vertex> project_polygon(std::span<camera_vertex const> const vertices, camera_projection const &view, projection_source const source) {
  /// Clip at the native near plane, retaining palette and texture interpolation
  std::vector<projected_vertex> result;
  if(vertices.empty()) return result;
  result.reserve(vertices.size() + 2);
  auto previous{&vertices.back()};
  bool previous_inside{inside_near_plane(previous->position, view, source)};
  for(auto const &current : vertices) {
    bool const current_inside{inside_near_plane(current.position, view, source)};
    if(current_inside != previous_inside) {
      auto const numerator{wrap_long(static_cast<int64_t>(view.near_distance) - previous->position.y)};
      auto const denominator{wrap_long(static_cast<int64_t>(current.position.y) - previous->position.y)};
      auto const x{interpolate(previous->position.x, current.position.x, numerator, denominator)};
      auto const z{interpolate(previous->position.z, current.position.z, numerator, denominator)};
      // Native intersections are already screen-scaled: near == 1 << shift is
      // assumed here, even if a caller supplies a nonstandard projection pair.
      result.push_back({
        .position{wrap_long(static_cast<int64_t>(view.centre.x) + x), wrap_long(static_cast<int64_t>(view.centre.y) - z)},
        .texture{interpolate(previous->texture.x, current.texture.x, numerator, denominator), interpolate(previous->texture.y, current.texture.y, numerator, denominator)},
        .shade{static_cast<uint16_t>(interpolate(previous->shade, current.shade, numerator, denominator))},
      });
    }
    if(current_inside) result.push_back({.position{project_point(current.position, view, source)}, .texture{vec2<int32_t>{current.texture}}, .shade{current.shade}});
    previous = &current;
    previous_inside = current_inside;
  }
  return result;
}

} // namespace earthsiege
