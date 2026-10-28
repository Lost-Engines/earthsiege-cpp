#pragma once
#include <array>
#include <cstdint>
#include <span>
#include "resources/artwork.h"

namespace earthsiege {

inline unsigned int constexpr texture_shade_count{8};
using texture_shade_maps = std::array<std::array<uint8_t, 256>, texture_shade_count>;

void apply_cockpit_map_shading(palette &target);
uint8_t shade_colour(std::span<uint16_t const> ramp, uint8_t intensity);
unsigned int texture_shade_index(uint8_t intensity);
texture_shade_maps make_texture_shades(palette const &source);

} // namespace earthsiege
