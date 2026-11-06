#pragma once
#include "render/framebuffer.h"
#include "render/projection.h"
#include "resources/artwork.h"
#include "resources/world.h"
#include "simulation/random_state.h"
namespace earthsiege {
class cloud_field {
  std::vector<vec3<int32_t>> field;
  std::vector<uint8_t> recycled;
  std::vector<bitmap> images;
  vec3<int32_t> previous,drift;
  int32_t top_texture{};
  int16_t active{};
  bool ready{},expansion{};
  int32_t altitude{10000};
public:
  cloud_field(world_environment const &,std::vector<bitmap>);
  /// Wind advances once per original draw, independently of simulation Q8 time.
  void draw(framebuffer &,vec3<int32_t> const &,vec3<uint16_t> const &,
            camera_projection const &,rectangle,uint8_t detail,random_state &);
  auto const &points() const { return field; }
  auto const &flags() const { return recycled; }
  auto const &anchor() const { return previous; }
  auto const &wind() const { return drift; }
  int32_t back_texture() const { return top_texture; }
  int16_t active_count() const { return active; }
  bool initialized() const { return ready; }
};
}
