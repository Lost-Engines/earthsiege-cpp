#include "fixed_math.h"
#include <algorithm>
#include <array>
#include <bit>

namespace earthsiege {

int16_t wrap_word(int32_t const value) {
  return std::bit_cast<int16_t>(static_cast<uint16_t>(value));
}
int32_t wrap_long(int64_t const value) {
  return std::bit_cast<int32_t>(static_cast<uint32_t>(value));
}

int32_t fast_distance_2d(vec2<int32_t> const &delta) {
  auto const x{delta.x < 0 ? wrap_long(-static_cast<int64_t>(delta.x)) : delta.x};
  auto const y{delta.y < 0 ? wrap_long(-static_cast<int64_t>(delta.y)) : delta.y};
  return wrap_long(static_cast<int64_t>(std::max(x, y)) + (std::min(x, y) >> 1));
}

int32_t fast_distance(vec3<int32_t> const &delta) {
  /// Retain the original ordered shifts rather than replacing them with a norm
  std::array<int32_t, 3> components;
  for(unsigned int axis{0}; axis != 3; ++axis) components[axis] = delta[axis] < 0 ? wrap_long(-static_cast<int64_t>(delta[axis])) : delta[axis];
  std::ranges::sort(components);
  return wrap_long(static_cast<int64_t>(components[2]) + (components[1] >> 2) + (components[1] >> 4) + (components[1] >> 5) + (components[0] >> 2));
}

} // namespace earthsiege
