#pragma once
#include <cstdint>
#include <span>
#include <vector>
#include "vectorstorm/vector/vector2.h"
#include "render/framebuffer.h"

namespace earthsiege {

struct polygon_span {
  int16_t left{0};
  int16_t right{0};                                                         // inclusive original endpoints
};

struct polygon_edges {
  int16_t first_row{0};
  std::vector<polygon_span> spans;
};

std::vector<vec2<int32_t>> clip_polygon_rectangle(std::span<vec2<int32_t> const> points, rectangle bounds);
polygon_edges make_polygon_edges(std::span<vec2<int32_t> const> clockwise_points);
enum class polygon_winding {
  detect,
  preserve,
};

void draw_solid_polygon(framebuffer &target, std::span<vec2<int32_t> const> points, rectangle bounds, uint8_t colour, polygon_winding winding = polygon_winding::detect);

} // namespace earthsiege
