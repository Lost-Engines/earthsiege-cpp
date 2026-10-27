#include "texture_raster.h"
#include <algorithm>
#include <cstdlib>
#include <stdexcept>
#include <vector>
#include "simulation/fixed_math.h"

namespace earthsiege {

namespace {

int32_t fixed_texture(int32_t const value) {
  return wrap_long(static_cast<int64_t>(value) * 65536);
}

int32_t divide_texture(int32_t const numerator, int32_t const denominator) {
  /// Native Q16 division rounds magnitudes to nearest, including negative ties
  if(denominator == 0) throw std::domain_error{"zero texture interpolation denominator"};
  auto const top{std::abs(static_cast<int64_t>(numerator)) * 65536};
  auto const bottom{std::abs(static_cast<int64_t>(denominator))};
  auto const magnitude{(top + bottom / 2) / bottom};
  return wrap_long((numerator < 0) != (denominator < 0) ? -magnitude : magnitude);
}

std::vector<projected_vertex> clip_texture_top(std::span<projected_vertex const> const vertices, int32_t const top) {
  std::vector<projected_vertex> result;
  for(size_t index{0}; index != vertices.size(); ++index) {
    auto const &a{vertices[index]};
    auto const &b{vertices[(index + 1) % vertices.size()]};
    bool const a_inside{a.position.y >= top};
    bool const b_inside{b.position.y >= top};
    if(a_inside) result.push_back(a);
    if(a_inside == b_inside) continue;
    auto const amount{wrap_long(static_cast<int64_t>(top) - a.position.y)};
    auto const distance{wrap_long(static_cast<int64_t>(b.position.y) - a.position.y)};
    auto const interpolate{[&](int32_t const from, int32_t const to){
      auto const difference{wrap_long(static_cast<int64_t>(to) - from)};
      return wrap_long(static_cast<int64_t>(from) + wrap_long(static_cast<int64_t>(difference) * amount) / distance);
    }};
    result.push_back({
      .position{interpolate(a.position.x, b.position.x), top},
      .texture{interpolate(a.texture.x, b.texture.x), interpolate(a.texture.y, b.texture.y)},
    });
  }
  return result;
}

class texture_edge {
  std::span<projected_vertex const> vertices;
  size_t end;
  int direction;
  unsigned int axis;
  size_t next{0};
  int32_t remaining{0};
  int32_t steps{0};
  vec2<int32_t> uv;
  vec2<int32_t> increment;
  int32_t extent{0};
  int32_t quotient{0};
  int32_t remainder{0};
  int32_t error{0};
  int sign{1};
  bool active{false};

  bool load(size_t start) {
    while(start != end) {
      next = direction > 0 ? (start + 1) % vertices.size() : (start + vertices.size() - 1) % vertices.size();
      auto const &a{vertices[start]};
      auto const &b{vertices[next]};
      steps = b.position[axis] - a.position[axis];
      if(steps != 0) {
        // Rounded animated quads can fold; native signed steps remain active
        // until the other edge ends, the spans cross, or the viewport ends.
        remaining = steps;
        for(unsigned int component{0}; component != 2; ++component) {
          uv[component] = fixed_texture(a.texture[component]);
          increment[component] = divide_texture(wrap_long(static_cast<int64_t>(fixed_texture(b.texture[component])) - uv[component]), fixed_texture(steps));
        }
        extent = a.position[1 - axis];
        auto const delta{b.position[1 - axis] - a.position[1 - axis]};
        sign = delta < 0 ? -1 : 1;
        quotient = sign * (std::abs(delta) / steps);
        remainder = std::abs(delta) % steps;
        error = delta < 0 ? 1 - steps : 0;
        return true;
      }
      start = next;
    }
    return false;
  }

public:
  texture_edge(std::span<projected_vertex const> const points, size_t const start, size_t const finish, int const walk, unsigned int const scan_axis)
    : vertices{points}, end{finish}, direction{walk}, axis{scan_axis}, active{load(start)} {
  }

  bool valid() const {
    return active;
  }

  int32_t coordinate() const {
    return extent;
  }

  vec2<int32_t> const &texture_position() const {
    return uv;
  }

  bool advance() {
    if(--remaining == 0) return active = load(next);
    for(unsigned int component{0}; component != 2; ++component) uv[component] = wrap_long(static_cast<int64_t>(uv[component]) + increment[component]);
    extent += quotient;
    error += remainder;
    if(error > 0) {
      extent += sign;
      error -= steps;
    }
    return true;
  }
};

} // anonymous namespace

void draw_textured_polygon(framebuffer &target, std::span<projected_vertex const> const vertices, bitmap const &texture, rectangle const bounds, std::span<uint8_t const> const translation, texture_scan_direction const direction) {
  /// Original affine mapper, switching scan direction for clipped cockpit views
  if(vertices.size() < 3 || bounds.left > bounds.right || bounds.top > bounds.bottom) return;
  if(!translation.empty() && translation.size() != 256) throw std::invalid_argument{"texture translation must contain 256 colours"};
  auto const clipped{clip_texture_top(vertices, bounds.top)};
  if(clipped.size() < 3) return;
  unsigned int const axis{direction == texture_scan_direction::rows ? 1U : 0U};
  int32_t const primary_first{axis ? bounds.top : bounds.left};
  int32_t const primary_last{axis ? bounds.bottom : bounds.right};
  int32_t const secondary_first{axis ? bounds.left : bounds.top};
  int32_t const secondary_last{axis ? bounds.right : bounds.bottom};
  auto const first{std::ranges::min_element(clipped, {}, [axis](auto const &p){return p.position[axis];})};
  auto const last{std::ranges::max_element(clipped, {}, [axis](auto const &p){return p.position[axis];})};
  auto const [top, bottom]{std::ranges::minmax_element(clipped, {}, [](auto const &p){return p.position.y;})};
  if(first->position[axis] == last->position[axis] || top->position.y == bottom->position.y || top->position.y > bounds.bottom) return;
  auto const start{static_cast<size_t>(first - clipped.begin())};
  auto const end{static_cast<size_t>(last - clipped.begin())};
  texture_edge upper{clipped, start, end, axis ? -1 : 1, axis};
  texture_edge lower{clipped, start, end, axis ? 1 : -1, axis};
  if(!upper.valid() || !lower.valid()) return;
  for(int32_t column{first->position[axis]}; column <= primary_last && upper.coordinate() <= lower.coordinate(); ++column) {
    if(column >= primary_first && upper.coordinate() <= secondary_last && lower.coordinate() >= secondary_first) {
      auto const count{lower.coordinate() - upper.coordinate() + 1};
      auto const row_first{std::max(secondary_first, upper.coordinate())};
      auto const row_last{std::min(secondary_last, lower.coordinate())};
      vec2<int32_t> delta;
      vec2<int32_t> uv;
      for(unsigned int component{0}; component != 2; ++component) {
        delta[component] = divide_texture(wrap_long(static_cast<int64_t>(lower.texture_position()[component]) - upper.texture_position()[component]), fixed_texture(count));
        uv[component] = wrap_long(static_cast<int64_t>(upper.texture_position()[component]) + (delta[component] >> 1) + static_cast<int64_t>(row_first - upper.coordinate()) * delta[component]);
      }
      for(int32_t row{row_first}; row <= row_last; ++row) {
        auto const u{wrap_long(static_cast<int64_t>(uv.x) + 32768) >> 16};
        auto const v{wrap_long(static_cast<int64_t>(uv.y) + 32768) >> 16};
        int64_t const index{u + static_cast<int64_t>(v) * texture.width};
        if(index < 0 || static_cast<uint64_t>(index) >= texture.pixels.size()) throw std::out_of_range{"texture sample outside bitmap"};
        auto const colour{texture.pixels[static_cast<size_t>(index)]};
        if(!translation.empty() || colour != 0) target.pixel(axis ? row : column, axis ? column : row, translation.empty() ? colour : translation[colour]);
        for(unsigned int component{0}; component != 2; ++component) uv[component] = wrap_long(static_cast<int64_t>(uv[component]) + delta[component]);
      }
    }
    if(!upper.advance() || !lower.advance()) break;
  }
}

} // namespace earthsiege
