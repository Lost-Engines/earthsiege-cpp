#pragma once
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>
#include "vectorstorm/vector/vector3.h"

namespace earthsiege {

struct scene_sort_segment {
  vec3<int32_t> first;
  vec3<int32_t> second;
};

struct scene_order_entry {
  vec3<int32_t> position;
  int32_t distance{0};
  std::optional<scene_sort_segment> segment;
};

/// Original two-pass painter tree: extended scenery first, point objects second
std::vector<size_t> scene_painter_order(std::span<scene_order_entry const> entries, vec3<int32_t> const &eye);

} // namespace earthsiege
