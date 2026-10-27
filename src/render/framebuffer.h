#pragma once
#include <array>
#include <cstdint>
#include <filesystem>
#include <string_view>

namespace earthsiege {

struct bitmap;
struct font;
struct palette;

inline unsigned int constexpr screen_width{320};
inline unsigned int constexpr screen_height{200};

struct rectangle {
  int left{};
  int top{};
  int right{};
  int bottom{};
  auto contains(int x, int y) const->bool;
};

class framebuffer {
  std::array<uint8_t, screen_width * screen_height> raster{};

public:
  void clear(uint8_t colour = 0);
  void pixel(int x, int y, uint8_t colour);
  void fill(rectangle area, uint8_t colour);
  void line(int x0, int y0, int x1, int y1, uint8_t colour);
  void image(bitmap const &source, int x, int y, bool transparent = false);
  void text(font const &face, std::string_view text, int x, int y);
  auto pixels() const->std::array<uint8_t, screen_width * screen_height> const &;
  void save(std::filesystem::path const &path, palette const &colours) const;
};

} // namespace earthsiege
