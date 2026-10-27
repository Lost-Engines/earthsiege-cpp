#include "polygon_raster.h"
#include <algorithm>
#include <array>
#include <cstdlib>
#include <limits>
#include <stdexcept>
#include "simulation/fixed_math.h"

namespace earthsiege {

std::vector<vec2<int32_t>> clip_polygon_rectangle(std::span<vec2<int32_t> const> const points, rectangle const bounds) {
  /// Two slab passes preserve the original outside-endpoint rounding direction
  std::vector<vec2<int32_t>> clipped{points.begin(), points.end()};
  std::array<std::array<int32_t, 2>, 2> const limits{{{bounds.left, bounds.right}, {bounds.top, bounds.bottom}}};
  if(bounds.left > bounds.right || bounds.top > bounds.bottom) return {};
  for(unsigned int axis{0}; axis != 2 && !clipped.empty(); ++axis) {
    auto const low{limits[axis][0]};
    auto const high{limits[axis][1]};
    auto const side{[&](vec2<int32_t> const &p){return p[axis] < low ? -1 : p[axis] > high ? 1 : 0;}};
    std::vector<vec2<int32_t>> result;
    result.reserve(clipped.size() + 2);
    auto const crossing{[&](vec2<int32_t> const &from, vec2<int32_t> const &to, int const outside){
      vec2<int32_t> p{from};
      p[axis] = outside < 0 ? low : high;
      auto const along{wrap_long(static_cast<int64_t>(p[axis]) - from[axis])};
      auto const across{wrap_long(static_cast<int64_t>(to[1 - axis]) - from[1 - axis])};
      auto const distance{wrap_long(static_cast<int64_t>(to[axis]) - from[axis])};
      if(distance == 0) throw std::domain_error{"viewport crossing has zero denominator"};
      p[1 - axis] = wrap_long(static_cast<int64_t>(from[1 - axis]) + static_cast<int64_t>(along) * across / distance);
      return p;
    }};
    auto const *previous{&clipped.back()};
    for(auto const &current : clipped) {
      auto const a{side(*previous)};
      auto const b{side(current)};
      if(a == 0 && b == 0) result.push_back(current);
      else if(a != b) {
        if(a) result.push_back(crossing(*previous, current, a));
        if(b) result.push_back(crossing(current, *previous, b));
        else result.push_back(current);
      }
      previous = &current;
    }
    clipped = std::move(result);
  }
  return clipped;
}

polygon_edges make_polygon_edges(std::span<vec2<int32_t> const> const points) {
  /// Walk both authored chains using the native side-dependent Bresenham rules
  polygon_edges result;
  if(points.empty()) return result;
  size_t top{0};
  size_t bottom{0};
  for(size_t index{points.size() - 1}; index != 0; --index) {
    auto const &p{points[index]};
    if(p.y < points[top].y || (p.y == points[top].y && p.x >= points[top].x)) top = index;
    if(p.y > points[bottom].y || (p.y == points[bottom].y && p.x <= points[bottom].x)) bottom = index;
  }
  auto const first{points[top].y};
  auto const last{points[bottom].y};
  if(first < std::numeric_limits<int16_t>::min() || last > std::numeric_limits<int16_t>::max()) throw std::out_of_range{"polygon rows exceed original edge table"};
  result.first_row = static_cast<int16_t>(first);
  result.spans.resize(static_cast<size_t>(last - first + 1));
  if(first == last) {
    result.spans[0] = {.left{wrap_word(points[bottom].x)}, .right{wrap_word(points[top].x)}};
    return result;
  }
  for(unsigned int side{0}; side != 2; ++side) {
    auto cursor{side ? top : bottom};
    auto const destination{side ? last : first};
    auto const put{[&](int32_t const x, int32_t const y){
      auto &span{result.spans.at(static_cast<size_t>(y - first))};
      if(side) span.right = wrap_word(x);
      else span.left = wrap_word(x);
    }};
    while(points[cursor].y != destination) {
      vec2<int32_t> a{points[cursor]};
      cursor = (cursor + 1) % points.size();
      vec2<int32_t> b{points[cursor]};
      auto const dx{std::abs(b.x - a.x)};
      auto const dy{std::abs(b.y - a.y)};
      if(dy == 0) {
        put(side ? std::max(a.x, b.x) : std::min(a.x, b.x), a.y);
      } else if(dx <= dy) {
        if(a.x > b.x) std::swap(a, b);
        auto x{a.x};
        auto y{a.y};
        int const step{b.y >= a.y ? 1 : -1};
        int error{2 * dx - dy};
        for(int row{0}; row <= dy; ++row) {
          put(x, y);
          if(error >= 0) {
            ++x;
            error -= 2 * dy;
          }
          error += 2 * dx;
          y += step;
        }
      } else {
        if((a.x < b.x) == static_cast<bool>(side)) std::swap(a, b);
        auto x{a.x};
        auto y{a.y};
        int const step_x{b.x >= a.x ? 1 : -1};
        int const step_y{b.y >= a.y ? 1 : -1};
        int error{2 * dy - dx};
        put(x, y);
        for(int column{0}; column != dx; ++column) {
          x += step_x;
          if(error >= 0) {
            y += step_y;
            error -= 2 * dx;
            put(x, y);
          }
          error += 2 * dy;
        }
      }
    }
  }
  return result;
}

void draw_solid_polygon(framebuffer &target, std::span<vec2<int32_t> const> const points, rectangle const bounds, uint8_t const colour) {
  auto clipped{clip_polygon_rectangle(points, bounds)};
  if(clipped.empty()) return;
  int64_t area{0};
  auto const *previous{&clipped.back()};
  for(auto const &p : clipped) {
    area += static_cast<int64_t>(previous->x) * p.y - static_cast<int64_t>(p.x) * previous->y;
    previous = &p;
  }
  if(area < 0) std::ranges::reverse(clipped);
  auto const edges{make_polygon_edges(clipped)};
  for(size_t row{0}; row != edges.spans.size(); ++row) {
    auto const &span{edges.spans[row]};
    auto const y{edges.first_row + static_cast<int>(row)};
    if(span.left <= span.right) target.fill({span.left, y, span.right, y}, colour);
  }
}

} // namespace earthsiege
