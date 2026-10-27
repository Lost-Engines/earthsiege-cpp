#pragma once
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include "resources/model.h"

namespace earthsiege {

struct scenery_collider {
  collision_heightfield const *field{nullptr};
  model_transform placement;
  int16_t radius{0};
};

struct scenery_hit {
  size_t object{0};
  int32_t distance{0};
};

/// Original moving-body occupancy ignores body radius and world altitude
int32_t scenery_contact_height(std::span<scenery_collider const> objects, vec3<int32_t> const &position);

int32_t scenery_height(collision_heightfield const &field, vec2<int32_t> const &point, int16_t radius = 0);
std::optional<vec3<int32_t>> intersect_heightfield(collision_heightfield const &field, vec3<int32_t> const &start, vec3<int32_t> const &end, int16_t radius = 0);
std::optional<scenery_hit> intersect_scenery(std::span<scenery_collider const> objects, model_transform const &ray, int32_t reach, int16_t radius = 0);

} // namespace earthsiege
