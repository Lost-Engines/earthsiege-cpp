#include "beam_effect.h"
#include <algorithm>
#include <stdexcept>
#include "simulation/fixed_math.h"

namespace earthsiege {

beam_effect make_beam_effect(int16_t const subtype, beam_path const &path, random_state &random) {
  std::array<int16_t, 6> constexpr widths{60, 30, 120, 20, 25, 30};
  std::array<uint8_t, 6> constexpr colours{10, 102, 10, 88, 88, 88};
  auto const index{static_cast<size_t>(subtype)};
  auto const width{widths.at(index)};
  beam_effect result{.position{vec3<int32_t>{path.start}}, .points{}, .faces{}, .colour{colours.at(index)}, .remaining{0}};
  if(subtype == 1) {
    vec3<int32_t> step;
    for(unsigned int axis{0}; axis != 3; ++axis) step[axis] = wrap_long(static_cast<int64_t>(path.end[axis]) - path.start[axis]);
    auto const distance{fast_distance(step)};
    auto const segments{static_cast<int8_t>((distance >> 10) + 1)};
    if(segments <= 0 || segments > 20) throw std::out_of_range{"beam exceeds original twenty-face index table"};
    if(distance != 0) {
      auto const factor{wrap_word((1024 * 65536) / distance)};
      for(unsigned int axis{0}; axis != 3; ++axis) step[axis] = wrap_word(static_cast<int32_t>((static_cast<int64_t>(factor) * step[axis] + 32768) >> 16));
    }
    auto centre{vec3<int32_t>{path.start}};
    for(int16_t segment{0}; segment <= segments; ++segment) {
      auto point{segment == segments ? vec3<int32_t>{path.end} : vec3<int32_t>{centre}};
      if(segment != 0) for(unsigned int axis{0}; axis != 3; ++axis) point[axis] = wrap_long(static_cast<int64_t>(point[axis]) + (random.next() & 127));
      auto upper{vec3<int32_t>{point}};
      upper.z = wrap_long(static_cast<int64_t>(upper.z) + width);
      result.points.push_back(upper); result.points.push_back(point);
      if(segment < segments) {
        auto const first{static_cast<uint16_t>(segment * 2)};
        result.faces.push_back({static_cast<uint16_t>(first + 1), first, static_cast<uint16_t>(first + 2), static_cast<uint16_t>(first + 3)});
      }
      for(unsigned int axis{0}; axis != 3; ++axis) centre[axis] = wrap_long(static_cast<int64_t>(centre[axis]) + step[axis]);
    }
  } else {
    for(unsigned int const axis : {2U, 0U}) for(auto const *point : {&path.start, &path.end}) for(int const sign : {1, -1}) {
      auto vertex{vec3<int32_t>{*point}};
      // Both ribbons end with vertical separation; only the second muzzle
      // pair is horizontal in the original constructor.
      auto const endpoint_axis{point == &path.end ? 2U : axis};
      vertex[endpoint_axis] = wrap_long(static_cast<int64_t>(vertex[endpoint_axis]) + sign * width);
      result.points.push_back(vertex);
    }
    result.faces = {{1, 0, 2, 3}, {5, 4, 6, 7}};
  }
  return result;
}

bool advance_beam_effect(beam_effect &beam, frame_ratio const ratio) {
  beam.remaining = std::max<int16_t>(0, wrap_word(beam.remaining - ratio));
  return beam.remaining == 0;
}

} // namespace earthsiege
