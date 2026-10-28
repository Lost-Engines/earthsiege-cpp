#pragma once
#include <array>
#include <cstdint>
#include <filesystem>
#include <string_view>
#include <vector>

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

struct scanline_span {
  int left;
  int right;
};

struct screen_region {
  int first_row{};
  std::vector<scanline_span> spans;
  bool rectangular{false};
  bool contains(int x, int y) const;
};

/// Inclusive window spans from an original ED cockpit clipping resource.
struct screen_mask {
  std::vector<screen_region> regions;
  void translate(int x, int y);
  std::array<std::vector<scanline_span>, screen_height> rows;
  bool contains(int x, int y) const;
};

class framebuffer {
  std::array<uint8_t, screen_width * screen_height> raster{};
  screen_mask const *mask{nullptr};

public:
  screen_mask const *clip() const;
  void clip(screen_mask const *value);
  void clear(uint8_t colour = 0);
  void pixel(int x, int y, uint8_t colour);
  void fill(rectangle area, uint8_t colour);
  void line(int x0, int y0, int x1, int y1, uint8_t colour);
  void image(bitmap const &source, int x, int y, bool transparent = false);
  void text(font const &face, std::string_view text, int x, int y);
  auto pixels() const->std::array<uint8_t, screen_width * screen_height> const &;
  void save(std::filesystem::path const &path, palette const &colours) const;
};

/// Keep cockpit clipping local to scenery drawing; instruments use their own bounds.
class scoped_framebuffer_clip {
  framebuffer &target;
  screen_mask const *previous;
public:
  scoped_framebuffer_clip(framebuffer &target, screen_mask const *mask);
  ~scoped_framebuffer_clip();
  scoped_framebuffer_clip(scoped_framebuffer_clip const &) = delete;
  scoped_framebuffer_clip &operator=(scoped_framebuffer_clip const &) = delete;
};

} // namespace earthsiege
