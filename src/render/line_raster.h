#pragma once
#include <cstdint>
#include "vectorstorm/vector/vector2.h"
#include "render/framebuffer.h"

namespace earthsiege {

enum class line_raster_mode { direct, clip_regions };

bool clip_line_rectangle(vec2<int32_t> &first, vec2<int32_t> &second, rectangle bounds);
void draw_model_line(framebuffer &target, vec2<int32_t> first, vec2<int32_t> second, rectangle bounds, uint8_t colour, line_raster_mode mode = line_raster_mode::direct);

} // namespace earthsiege
