#pragma once
#include "render/background_bands.h"
#include "resources/artwork.h"
#include <span>

namespace earthsiege {
struct panorama_placement {
 size_t frame{};
 vec2<int32_t> position{};
 uint16_t angle{};
 bool rotated{};
};
std::vector<panorama_placement> place_panorama(std::span<bitmap const>,background_bands const &,background_view const &,uint16_t heading,int horizon_offset);
void draw_panorama(framebuffer &,std::span<bitmap const>,std::span<panorama_placement const>,rectangle bounds);
} // namespace earthsiege
