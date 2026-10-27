#pragma once
#include <cstdint>
#include "vectorstorm/vector/vector2.h"
#include "render/framebuffer.h"

namespace earthsiege {

bool clip_line_rectangle(vec2<int32_t> &first, vec2<int32_t> &second, rectangle bounds);
void draw_model_line(framebuffer &target, vec2<int32_t> first, vec2<int32_t> second, rectangle bounds, uint8_t colour);

} // namespace earthsiege
