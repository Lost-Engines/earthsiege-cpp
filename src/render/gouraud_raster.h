#pragma once
#include <array>
#include <cstdint>
#include <span>
#include <vector>
#include "render/framebuffer.h"
#include "render/projection.h"

namespace earthsiege {

struct palette_band {
  std::array<vec2<int32_t>, 4> points{vec2<int32_t>{0, 0}};
  uint8_t vertex_count{0};
  uint8_t colour{0};
};

std::vector<palette_band> split_gouraud_triangle(std::array<vec2<int32_t>, 3> const &points, std::array<uint8_t, 3> colours);
void draw_gouraud_triangle(framebuffer &target, std::array<vec2<int32_t>, 3> const &points, std::array<uint8_t, 3> colours, rectangle bounds);
void draw_gouraud_polygon(framebuffer &target, std::span<projected_vertex const> vertices, std::span<uint16_t const> ramp, rectangle bounds);

} // namespace earthsiege
