#pragma once
#include <array>
#include <cstdint>
#include <string_view>
#include <vector>
#include "resources/binary_reader.h"

namespace earthsiege {

struct bitmap {
  unsigned int width{};
  unsigned int height{};
  uint8_t flags{};
  std::vector<uint8_t> pixels;
};

struct palette {
  std::array<std::array<uint8_t, 3>, 256> dac_colours{};
  std::array<uint32_t, 256> colours{};
  std::vector<std::vector<uint16_t>> shade_ramps;
};

struct glyph {
  unsigned int width{};
  std::vector<uint8_t> pixels;
};

struct font {
  unsigned int first_character{};
  unsigned int height{};
  int baseline{};
  std::vector<glyph> glyphs;
  auto text_width(std::string_view text) const->int;
};

auto read_bitmaps(bytes data)->std::vector<bitmap>;
auto read_palette(bytes data)->palette;
auto read_font(bytes data)->font;

} // namespace earthsiege
