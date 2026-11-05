#pragma once

#include <algorithm>
#include <array>
#include <cmath>

namespace earthsiege {

/// One transform for presentation, mouse input and cursor warps, including HiDPI.
struct viewport {
  int x{};
  int y{};
  int width{1};
  int height{1};

  static auto fit(int const width, int const height)->viewport {
    double const available{std::min(width / 320.0, height / 200.0)};
    double const scale{available >= 1 ? std::floor(available) : available};
    int const w{std::max(1, static_cast<int>(320 * scale))};
    int const h{std::max(1, static_cast<int>(200 * scale))};
    return {(width - w) / 2, (height - h) / 2, w, h};
  }

  auto logical(double const px, double const py, int const window_width, int const window_height,
               int const buffer_width, int const buffer_height) const->std::array<int, 2> {
    if(window_width <= 0 || window_height <= 0) return {-1, -1};
    return {static_cast<int>(std::floor((px * buffer_width / window_width - x) * 320 / width)),
            static_cast<int>(std::floor((py * buffer_height / window_height - y) * 200 / height))};
  }

  auto cursor(int const px, int const py, int const window_width, int const window_height,
              int const buffer_width, int const buffer_height) const->std::array<double, 2> {
    return {(x + (px + 0.5) * width / 320) * window_width / std::max(1.0, double(buffer_width)),
            (y + (py + 0.5) * height / 200) * window_height / std::max(1.0, double(buffer_height))};
  }
};

} // namespace earthsiege
