#include "framebuffer.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <stdexcept>
#include "resources/artwork.h"

namespace earthsiege {

auto rectangle::contains(int x, int y) const->bool {
  return x >= left && x <= right && y >= top && y <= bottom;
}

bool screen_region::contains(int x, int y) const {
  int const row{y - first_row};
  if(row < 0 || static_cast<size_t>(row) >= spans.size()) return false;
  auto const span{spans[static_cast<size_t>(row)]};
  return x >= span.left && x <= span.right;
}

void screen_mask::translate(int x, int y) {
  for(auto &row : rows) row.clear();
  for(auto &region : regions) {
    region.first_row += y;
    for(size_t i{}; i < region.spans.size(); ++i) {
      auto &span{region.spans[i]}; span.left += x; span.right += x;
      int const row{region.first_row + static_cast<int>(i)};
      if(row < 0 || row >= static_cast<int>(screen_height)) continue;
      scanline_span const visible{std::max(0, span.left), std::min(static_cast<int>(screen_width) - 1, span.right)};
      if(visible.left <= visible.right) rows[static_cast<size_t>(row)].push_back(visible);
    }
  }
}

bool screen_mask::contains(int x, int y) const {
  if(y < 0 || y >= static_cast<int>(rows.size())) return false;
  return std::ranges::any_of(rows[static_cast<size_t>(y)], [x](scanline_span span){ return x >= span.left && x <= span.right; });
}

screen_mask const *framebuffer::clip() const { return mask; }
void framebuffer::clip(screen_mask const *value) { mask = value; }
scoped_framebuffer_clip::scoped_framebuffer_clip(framebuffer &buffer, screen_mask const *value) : target{buffer}, previous{buffer.clip()} { target.clip(value); }
scoped_framebuffer_clip::~scoped_framebuffer_clip() { target.clip(previous); }

void framebuffer::clear(uint8_t colour) { raster.fill(colour); }

void framebuffer::pixel(int x, int y, uint8_t colour) {
  if(x >= 0 && y >= 0 && x < static_cast<int>(screen_width) && y < static_cast<int>(screen_height) && (!mask || mask->contains(x, y))) {
    raster[static_cast<unsigned int>(y) * screen_width + static_cast<unsigned int>(x)] = colour;
  }
}

void framebuffer::fill(rectangle area, uint8_t colour) {
  area.left = std::max(0, area.left);
  area.top = std::max(0, area.top);
  area.right = std::min(static_cast<int>(screen_width) - 1, area.right);
  area.bottom = std::min(static_cast<int>(screen_height) - 1, area.bottom);
  if(area.left > area.right || area.top > area.bottom) return;
  for(int y{area.top}; y <= area.bottom; ++y) {
    auto const span_fill{[&](int left, int right){
      if(left <= right) std::fill_n(raster.begin() + y * screen_width + left, right - left + 1, colour);
    }};
    if(mask) for(auto const span : mask->rows[static_cast<size_t>(y)]) span_fill(std::max(area.left, span.left), std::min(area.right, span.right));
    else span_fill(area.left, area.right);
  }
}

void framebuffer::line(int x0, int y0, int x1, int y1, uint8_t colour) {
  /// Rasterise inclusive line endpoints in native screen coordinates.
  int const dx{std::abs(x1 - x0)};
  int const dy{-std::abs(y1 - y0)};
  int const sx{x0 < x1 ? 1 : -1};
  int const sy{y0 < y1 ? 1 : -1};
  int error{dx + dy};
  for(;;) {
    pixel(x0, y0, colour);
    if(x0 == x1 && y0 == y1) break;
    int const twice{error * 2};
    if(twice >= dy) { error += dy; x0 += sx; }
    if(twice <= dx) { error += dx; y0 += sy; }
  }
}

void framebuffer::image(bitmap const &source, int x, int y, bool transparent) {
  /// Blit original palette indices; zero is the transparent sprite colour.
  for(unsigned int row{}; row < source.height; ++row) {
    for(unsigned int column{}; column < source.width; ++column) {
      auto const colour{source.pixels[row * source.width + column]};
      if(!transparent || colour) pixel(x + static_cast<int>(column), y + static_cast<int>(row), colour);
    }
  }
}

void framebuffer::text(font const &face, std::string_view value, int x, int y) {
  for(unsigned char const character : value) {
    if(character < face.first_character || character - face.first_character >= face.glyphs.size()) continue;
    auto const &letter{face.glyphs[character - face.first_character]};
    for(unsigned int row{}; row < face.height; ++row) {
      for(unsigned int column{}; column < letter.width; ++column) {
        auto const colour{letter.pixels[row * letter.width + column]};
        if(colour) pixel(x + static_cast<int>(column), y + static_cast<int>(row) - face.baseline, colour);
      }
    }
    x += static_cast<int>(letter.width);
  }
}

auto framebuffer::pixels() const->std::array<uint8_t, screen_width * screen_height> const & { return raster; }

void framebuffer::save(std::filesystem::path const &path, palette const &colours) const {
  /// Save an unscaled frame for comparisons, independent of window size.
  std::ofstream file{path, std::ios::binary};
  file << "P6\n" << screen_width << ' ' << screen_height << "\n255\n";
  for(auto const index : raster) {
    auto const rgb{colours.colours[index]};
    std::array const pixel{static_cast<char>(rgb >> 16), static_cast<char>(rgb >> 8), static_cast<char>(rgb)};
    file.write(pixel.data(), pixel.size());
  }
  if(!file) throw std::runtime_error{"cannot write screenshot " + path.string()};
}

} // namespace earthsiege
