#include "scenery_collision.h"
#include <algorithm>
#include <stdexcept>
#include "simulation/fixed_math.h"
#include "simulation/model_math.h"

namespace earthsiege {
namespace {

vec2<int32_t> origin_offset(collision_heightfield const &field) {
  unsigned int const shift{static_cast<unsigned int>(field.cell_shift) & 31};
  return vec2<int32_t>{
    wrap_word(wrap_long(static_cast<int64_t>(field.origin.x) * (int64_t{1} << shift))),
    wrap_word(wrap_long(static_cast<int64_t>(field.origin.y) * (int64_t{1} << shift))),
  };
}

int32_t grid_height(collision_heightfield const &field, vec2<int32_t> const &point, int16_t const radius) {
  unsigned int const shift{static_cast<unsigned int>(field.cell_shift) & 31};
  bool const area{radius > wrap_long(int64_t{1} << ((shift - 1) & 31))};
  int const extent{area ? radius : 0};
  int const x0{wrap_word(wrap_long(static_cast<int64_t>(point.x) - extent) >> shift)};
  int const y0{wrap_word(wrap_long(static_cast<int64_t>(point.y) - extent) >> shift)};
  int const x1{wrap_word(wrap_long(static_cast<int64_t>(point.x) + extent) >> shift)};
  int const y1{wrap_word(wrap_long(static_cast<int64_t>(point.y) + extent) >> shift)};
  int32_t height{0};
  for(int y{std::max(0, y0)}; y <= std::min(field.size.y - 1, y1); ++y) {
    for(int x{std::max(0, x0)}; x <= std::min(field.size.x - 1, x1); ++x) {
      auto const index{field.cells.at(static_cast<size_t>(y * field.size.x + x))};
      if(!area) return field.heights[index];
      height = std::max(height, field.heights[index]);
    }
  }
  return height;
}

vec3<int32_t> difference(vec3<int32_t> const &a, vec3<int32_t> const &b) {
  return vec3<int32_t>{wrap_long(static_cast<int64_t>(a.x) - b.x), wrap_long(static_cast<int64_t>(a.y) - b.y), wrap_long(static_cast<int64_t>(a.z) - b.z)};
}

vec3<int32_t> transformed(vec3<int32_t> const &point, model_transform const &transform) {
  auto result{rotate_vector(point, transform)};
  for(unsigned int axis{0}; axis != 3; ++axis) result[axis] = wrap_long(static_cast<int64_t>(result[axis]) + transform.translation[axis]);
  return result;
}

} // anonymous namespace

int32_t scenery_height(collision_heightfield const &field, vec2<int32_t> const &point, int16_t const radius) {
  if(field.size.x == 0) return 0;
  auto const origin{origin_offset(field)};
  return grid_height(field, vec2<int32_t>{wrap_long(static_cast<int64_t>(point.x) + origin.x), wrap_long(static_cast<int64_t>(point.y) + origin.y)}, radius);
}

std::optional<vec3<int32_t>> intersect_heightfield(collision_heightfield const &field, vec3<int32_t> const &start, vec3<int32_t> const &end, int16_t const radius) {
  /// The original tests evenly spaced samples, excluding the segment endpoint
  if(field.size.x == 0) return std::nullopt;
  auto const maximum{field.heights.back()};
  if((start.z >= maximum && end.z >= maximum) || (start.z < 0 && end.z < 0)) return std::nullopt;
  unsigned int const shift{static_cast<unsigned int>(field.cell_shift) & 31};
  auto const spacing{wrap_word(wrap_long((int64_t{1} << shift) + radius))};
  if(spacing <= 0) throw std::invalid_argument{"heightfield sampling requires positive step spacing"};
  auto const delta{difference(end, start)};
  auto const length{fast_distance(delta)};
  if(length == 0) return std::nullopt;
  auto const count{wrap_word(length / spacing)};
  auto const scale{wrap_word(wrap_long((static_cast<int64_t>(spacing) << 16) / length))};
  vec3<int32_t> step;
  for(unsigned int axis{0}; axis != 3; ++axis) step[axis] = wrap_word(wrap_long((static_cast<int64_t>(delta[axis]) * scale + 32768) >> 16));
  auto const origin{origin_offset(field)};
  vec3<int32_t> point{wrap_long(static_cast<int64_t>(start.x) + origin.x), wrap_long(static_cast<int64_t>(start.y) + origin.y), start.z};
  for(int index{0}; index < count; ++index) {
    if(point.z < grid_height(field, vec2<int32_t>{point.x, point.y}, radius)) {
      return vec3<int32_t>{wrap_long(static_cast<int64_t>(point.x) - origin.x), wrap_long(static_cast<int64_t>(point.y) - origin.y), point.z};
    }
    for(unsigned int axis{0}; axis != 3; ++axis) point[axis] = wrap_long(static_cast<int64_t>(point[axis]) + step[axis]);
  }
  return std::nullopt;
}

std::optional<scenery_hit> intersect_scenery(std::span<scenery_collider const> const objects, model_transform const &ray, int32_t reach, int16_t const radius) {
  /// Preserve candidate order and shortening after a hit, including the early near-hit exit
  auto const inverse_ray{inverse_transform(ray)};
  std::optional<scenery_hit> result;
  int constexpr near_hit_distance{500};
  for(size_t index{0}; index != objects.size(); ++index) {
    auto const &object{objects[index]};
    if(!object.field) continue;
    int const broad_radius{object.radius + radius};
    if(fast_distance(difference(object.placement.translation, ray.translation)) > wrap_long(static_cast<int64_t>(broad_radius) + reach)) continue;
    auto const ray_local{transformed(object.placement.translation, inverse_ray)};
    if(ray_local.x <= -broad_radius || ray_local.x >= broad_radius || ray_local.y <= -broad_radius || ray_local.y >= wrap_long(static_cast<int64_t>(broad_radius) + reach)) continue;
    auto const inverse_object{inverse_transform(object.placement)};
    auto const start{transformed(ray.translation, inverse_object)};
    auto const end{transformed(transformed(vec3<int32_t>{0, reach, 0}, ray), inverse_object)};
    auto const hit{intersect_heightfield(*object.field, start, end, radius)};
    if(!hit) continue;
    reach = fast_distance(difference(*hit, start));
    result = scenery_hit{.object{index}, .distance{reach}};
    if(reach < near_hit_distance) break;
  }
  return result;
}

} // namespace earthsiege
