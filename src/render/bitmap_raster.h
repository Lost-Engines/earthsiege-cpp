#pragma once
#include "vectorstorm/vector/vector2.h"
#include "render/framebuffer.h"
#include "resources/artwork.h"

namespace earthsiege {

void draw_scaled_bitmap(framebuffer &target, bitmap const &source, vec2<int32_t> position, vec2<int32_t> size, rectangle bounds);

// GO uses endpoint-aligned sampling; DBSIM uses the later scaler above.
void draw_shell_scaled_bitmap(framebuffer &, bitmap const &, vec2<int32_t> position, vec2<int32_t> size, rectangle bounds);

} // namespace earthsiege
