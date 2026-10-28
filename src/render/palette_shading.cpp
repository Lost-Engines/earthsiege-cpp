#include "palette_shading.h"
#include <algorithm>
#include <numeric>
#include <stdexcept>
#include "simulation/fixed_math.h"

namespace earthsiege {

void apply_cockpit_map_shading(palette &target) {
  for(size_t ramp{}; ramp < 16; ramp += 2) {
    auto &values{target.shade_ramps.at(ramp)};
    values.resize(16);
    std::iota(values.begin(), values.end(), uint16_t{16});
  }
}

uint8_t shade_colour(std::span<uint16_t const> const ramp, uint8_t const intensity) {
  if(ramp.empty()) throw std::invalid_argument{"empty palette shade ramp"};
  auto const colour{ramp[(intensity * ramp.size()) >> 8]};
  if(colour > 255) throw std::out_of_range{"shade ramp exceeds palette"};
  return static_cast<uint8_t>(colour);
}

unsigned int texture_shade_index(uint8_t const intensity) {
  return (texture_shade_count - 1) * intensity / 255;
}

texture_shade_maps make_texture_shades(palette const &source) {
  /// Generate native eight-level texture translations in six-bit VGA space
  texture_shade_maps result;
  for(unsigned int level{0}; level != texture_shade_count; ++level) {
    for(unsigned int index{0}; index != 256; ++index) {
      std::array<int16_t, 3> scale{0, 0, 0};
      bool shaded{false};
      for(auto const &ramp : source.shade_ramps) {
        if(std::ranges::find(ramp, index) == ramp.end()) continue;
        shaded = true;                                                        // last containing ramp wins
        auto const reference{((ramp.size() - 1) * 650 + 512) >> 10};
        auto const shade{(ramp.size() - 1) * level / (texture_shade_count - 1)};
        auto const &reference_colour{source.dac_colours.at(ramp[reference])};
        auto const &selected_colour{source.dac_colours.at(ramp[shade])};
        for(unsigned int axis{0}; axis != 3; ++axis) {
          scale[axis] = reference_colour[axis] ? wrap_word((selected_colour[axis] << 10) / reference_colour[axis]) : 0;
        }
      }
      if(!shaded) {
        result[level][index] = static_cast<uint8_t>(index);
        continue;
      }
      std::array<int16_t, 3> target;
      for(unsigned int axis{0}; axis != 3; ++axis) target[axis] = wrap_word((source.dac_colours[index][axis] * scale[axis] + 512) >> 10);
      int16_t nearest{1000};
      bool found{false};
      for(unsigned int candidate{0}; candidate != 256; ++candidate) {
        vec3<int32_t> delta;
        for(unsigned int axis{0}; axis != 3; ++axis) delta[axis] = wrap_word(target[axis] - source.dac_colours[candidate][axis]);
        auto const distance{wrap_word(fast_distance(delta))};
        if(distance < nearest) {
          nearest = distance;
          result[level][index] = static_cast<uint8_t>(candidate);
          found = true;                                                        // strict comparison retains first tie
        }
      }
      if(!found) throw std::domain_error{"texture shade has no palette match within native search range"};
    }
  }
  return result;
}

} // namespace earthsiege
