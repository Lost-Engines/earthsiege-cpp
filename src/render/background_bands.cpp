#include "background_bands.h"
#include <algorithm>
#include <stdexcept>
#include <utility>
#include "render/polygon_raster.h"
#include "simulation/fixed_math.h"
#include "simulation/model_math.h"

namespace earthsiege {
namespace {

int32_t multiply_shift(int32_t const first, int32_t const second, unsigned int const shift) {
  return wrap_long(static_cast<int64_t>(first) * second) >> shift;
}

void append(background_bands &result, background_primitive_kind const kind, int32_t const colour, std::vector<vec2<int32_t>> points) {
  result.primitives.push_back(background_primitive{.kind{kind}, .colour{static_cast<uint32_t>(colour)}, .points{std::move(points)}});
}

void bands(background_bands &result, background_view const &view, int32_t const first_colour, int32_t const count, int32_t const step, int32_t const first_width, bool const sky) {
  auto first{result.horizon[0]};
  auto second{result.horizon[1]};
  auto const &direction{result.horizon[2]};
  if(sky ? first.y < view.bounds.top && second.y < view.bounds.top : first.y > view.bounds.bottom && second.y > view.bounds.bottom) return;
  auto colour{first_colour + count - 1};
  int const sign{sky ? 1 : -1};
  auto const dx{multiply_shift(-direction.x, step, 14)};
  auto const dy{multiply_shift(direction.y, step, 14)};
  auto initial_dy{-(first_width >> 1)};
  if(view.roll == 0) {
    for(int32_t index{0}; index < count - 1; ++index) {
      /// The original rectangle path also shifts one offscreen horizontal edge
      if(sky) {
        first.x += initial_dy;
        first.y += initial_dy;
      } else {
        second.x -= initial_dy;
        second.y -= initial_dy;
      }
      initial_dy = dy;
      append(result, background_primitive_kind::rectangle, colour, {vec2<int32_t>{first}, vec2<int32_t>{second}});
      if(sky) second.y = first.y;
      else first.y = second.y;
      --colour;
    }
    if(sky) first.y = view.bounds.top;
    else second.y = view.bounds.bottom;
    if(first.y <= second.y) append(result, background_primitive_kind::rectangle, colour, {vec2<int32_t>{first}, vec2<int32_t>{second}});
    return;
  }
  auto const kind{sky ? background_primitive_kind::sky_polygon : background_primitive_kind::ground_polygon};
  for(int32_t index{0}; index < count - 1; ++index) {
    vec2<int32_t> const next_first{first.x + sign * dx, first.y + sign * initial_dy};
    vec2<int32_t> const next_second{second.x + sign * dx, second.y + sign * initial_dy};
    initial_dy = dy;
    append(result, kind, colour, {vec2<int32_t>{first}, vec2<int32_t>{second}, vec2<int32_t>{next_second}, vec2<int32_t>{next_first}});
    first = next_first;
    second = next_second;
    --colour;
  }
  std::vector<vec2<int32_t>> points{vec2<int32_t>{first}, vec2<int32_t>{second}};
  vec2<int32_t> const top_left{view.bounds.left, view.bounds.top};
  vec2<int32_t> const bottom_right{view.bounds.right, view.bounds.bottom};
  if(sky) {
    if(first.y < second.y) {
      if(second.x < view.bounds.right) points.push_back(bottom_right);
      points.emplace_back(view.bounds.right, view.bounds.top);
      if(first.x < view.bounds.left) points.push_back(top_left);
    } else {
      if(second.x > view.bounds.right) points.emplace_back(view.bounds.right, view.bounds.top);
      points.push_back(top_left);
      if(first.x > view.bounds.left) points.emplace_back(view.bounds.left, view.bounds.bottom);
    }
  } else {
    if(first.y < second.y) {
      if(second.x > view.bounds.right) points.push_back(bottom_right);
      points.emplace_back(view.bounds.left, view.bounds.bottom);
      if(first.x > view.bounds.left) points.push_back(top_left);
    } else {
      if(second.x < view.bounds.right) points.emplace_back(view.bounds.right, view.bounds.top);
      points.push_back(bottom_right);
      if(first.x < view.bounds.left) points.emplace_back(view.bounds.left, view.bounds.bottom);
    }
  }
  append(result, kind, colour, std::move(points));
}

} // anonymous namespace

background_bands make_background_bands(world_environment const &environment, background_view const &view) {
  auto const &style{environment.background};
  auto const pitch{static_cast<uint16_t>(-view.pitch)};
  auto const pitch_sine{model_cosine(static_cast<uint16_t>(pitch - angle_quarter_turn))};
  auto pitch_cosine{model_cosine(pitch)};
  if(pitch_cosine == 0) pitch_cosine = model_cosine(static_cast<uint16_t>(pitch - 1));
  if(pitch_cosine == 0) throw std::domain_error{"original horizon cosine divisor is zero"};
  auto const sine{model_cosine(static_cast<uint16_t>(view.roll - angle_quarter_turn))};
  auto const cosine{model_cosine(view.roll)};
  auto const distance{wrap_long(static_cast<int64_t>(pitch_sine) * (int64_t{1} << view.projection_shift)) / pitch_cosine};
  auto const x{cosine >> 6};
  auto const y{sine >> 6};
  auto const offset_x{multiply_shift(distance, -y, 8)};
  auto const offset_y{multiply_shift(distance, x, 8)};
  background_bands result;
  result.horizon[0] = vec2<int32_t>{-x + offset_x + view.centre.x, view.centre.y - (-y + offset_y) + style.horizon_offset};
  result.horizon[1] = vec2<int32_t>{x + offset_x + view.centre.x, view.centre.y - (y + offset_y) + style.horizon_offset};
  auto const scroll_x{multiply_shift(cosine, view.offset.y, 14)};
  auto const scroll_y{multiply_shift(-sine, view.offset.x, 14)};
  for(unsigned int index{0}; index != 2; ++index) {
    result.horizon[index].x += scroll_x + view.offset.x;
    result.horizon[index].y += scroll_y + view.offset.y + (style.horizon_offset >> 1);
  }
  /// The original signed upper-limit comparison can never invert a zero-extended pitch
  result.horizon[2] = vec2<int32_t>{sine, -cosine};
  bands(result, view, style.sky_palette_start, style.sky_bands, style.sky_step, style.horizon_band_width, true);
  ++result.horizon[0].y;
  ++result.horizon[1].y;
  bands(result, view, style.ground_palette_start, style.ground_bands, style.ground_step, style.horizon_band_width, false);
  return result;
}

void draw_background_bands(framebuffer &target, background_bands const &source, rectangle const bounds) {
  for(auto const &primitive : source.primitives) {
    auto const colour{static_cast<uint8_t>(primitive.colour)};
    if(primitive.kind == background_primitive_kind::rectangle) {
      auto const &first{primitive.points.at(0)};
      auto const &second{primitive.points.at(1)};
      target.fill({std::max(first.x, bounds.left), std::max(first.y, bounds.top), std::min(second.x, bounds.right), std::min(second.y, bounds.bottom)}, colour);
    } else {
      auto points{primitive.points};
      if(primitive.kind == background_primitive_kind::sky_polygon) std::ranges::reverse(points);
      draw_solid_polygon(target, points, bounds, colour, polygon_winding::preserve);
    }
  }
}

} // namespace earthsiege
