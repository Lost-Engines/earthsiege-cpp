#pragma once
#include <array>
#include <cstdint>
#include <vector>
#include "render/framebuffer.h"
#include "resources/world.h"
#include "vectorstorm/vector/vector2.h"

namespace earthsiege {

struct background_view {
  uint16_t pitch{0};
  uint16_t roll{0};
  unsigned int projection_shift{8};
  vec2<int32_t> centre{160, 100};
  vec2<int32_t> offset;
  rectangle bounds{0, 0, 319, 199};
};

enum class background_primitive_kind : uint8_t {
  rectangle,
  sky_polygon,
  ground_polygon,
};

struct background_primitive {
  background_primitive_kind kind{background_primitive_kind::rectangle};
  uint32_t colour{0};
  std::vector<vec2<int32_t>> points;
};

struct background_bands {
  std::array<vec2<int32_t>, 3> horizon;                                         // endpoints followed by the band direction
  std::vector<background_primitive> primitives;
};

/// Original low-detail sky/ground gradients, before panorama, clouds and ground texture
background_bands make_background_bands(world_environment const &environment, background_view const &view, bool include_ground = true);
void draw_background_bands(framebuffer &target, background_bands const &bands, rectangle bounds);

} // namespace earthsiege
