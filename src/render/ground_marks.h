#pragma once
#include "render/framebuffer.h"
#include "render/projection.h"
#include "resources/world.h"
#include "simulation/random_state.h"

namespace earthsiege {
/// Original 0x26e8c mark field, including lazy initialization and recycling.
class ground_marks {
  std::vector<vec3<int32_t>> field;
  vec3<int32_t> previous;
  int16_t colour;
  int16_t active{};
  bool ready{};
public:
  explicit ground_marks(world_environment const &environment);
  void draw(framebuffer &,vec3<int32_t> const &position,vec3<uint16_t> const &angles,
            camera_projection const &,rectangle bounds,uint8_t detail,random_state &);
  std::span<vec3<int32_t> const> points() const { return field; }
  vec3<int32_t> const &anchor() const { return previous; }
  int16_t active_count() const { return active; }
  bool initialized() const { return ready; }
};
}
