#pragma once
#include <array>
#include <cstdint>
#include <string_view>
#include <vector>
#include "resources/binary_reader.h"

namespace earthsiege {

struct bitmap {
  unsigned int width{0};
  unsigned int height{0};
  uint8_t flags{0};
  std::vector<uint8_t> pixels;
};

struct palette {
  std::array<std::array<uint8_t, 3>, 256> dac_colours{};
  std::array<uint32_t, 256> colours{};
  std::vector<std::vector<uint16_t>> shade_ramps;
};

struct glyph {
  unsigned int width{0};
  std::vector<uint8_t> pixels;
};

struct font {
  unsigned int first_character{0};
  unsigned int height{0};
  int baseline{0};
  std::vector<glyph> glyphs;
  auto text_width(std::string_view text) const->int;
};

/// Six-bit VGA components expanded like the original DOSBox reference output.
auto display_colour(std::array<uint8_t, 3> const &dac)->uint32_t;
auto read_bitmaps(bytes data)->std::vector<bitmap>;
auto read_palette(bytes data)->palette;
auto read_font(bytes data)->font;

} // namespace earthsiege
