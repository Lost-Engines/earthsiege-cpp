#pragma once
#include <cstdint>
#include <span>
#include "render/framebuffer.h"
#include "render/projection.h"
#include "resources/artwork.h"

namespace earthsiege {

enum class texture_scan_direction {
  columns,
  rows,
};

void draw_textured_polygon(framebuffer &target, std::span<projected_vertex const> vertices, bitmap const &texture, rectangle bounds, std::span<uint8_t const> translation = {}, texture_scan_direction direction = texture_scan_direction::columns);

} // namespace earthsiege
