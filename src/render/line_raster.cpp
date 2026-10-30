#include "line_raster.h"
#include <algorithm>
#include <cstdlib>
#include "simulation/fixed_math.h"

namespace earthsiege {

bool clip_line_rectangle(vec2<int32_t> &first, vec2<int32_t> &second, rectangle const bounds) {
  /// The native clipper always makes the outside endpoint first before rounding
  auto const boundary{[&](unsigned int const axis, int32_t const edge, bool const maximum){
    auto const outside{[&](vec2<int32_t> const &point){ return maximum ? point[axis] > edge : point[axis] < edge; }};
    if(!outside(first)) {
      if(!outside(second)) return true;
      std::swap(first, second);
    } else if(outside(second)) return false;
    auto const other{1U - axis};
    auto const denominator{wrap_long(static_cast<int64_t>(first[axis]) - second[axis])};
    auto const change{static_cast<int64_t>(wrap_long(static_cast<int64_t>(first[other]) - second[other])) * wrap_long(static_cast<int64_t>(edge) - first[axis]) / denominator};
    first[other] = wrap_long(static_cast<int64_t>(first[other]) + change);
    first[axis] = edge;
    return true;
  }};
  return boundary(1, bounds.top, false) && boundary(0, bounds.left, false) && boundary(1, bounds.bottom, true) && boundary(0, bounds.right, true);
}

namespace {
// Original 7d358 uses centred Bresenham spans for clip-region lists. The
// unmasked VGA writer below instead accumulates (delta + 1) endpoint steps.
void draw_region_line(framebuffer &target,vec2<int32_t> first,vec2<int32_t> second,
                      rectangle bounds,uint8_t colour,screen_region const &region){
  if(region.rectangular&&!region.spans.empty()){
    bounds.left=std::max(bounds.left,region.spans.front().left);
    bounds.right=std::min(bounds.right,region.spans.front().right);
    bounds.top=std::max(bounds.top,region.first_row);
    bounds.bottom=std::min(bounds.bottom,region.first_row+static_cast<int>(region.spans.size())-1);
  }
  if(bounds.left>bounds.right||bounds.top>bounds.bottom||!clip_line_rectangle(first,second,bounds))return;
  if(second.x<first.x)std::swap(first,second);
  int const dx{second.x-first.x},dy{std::abs(second.y-first.y)};
  int const direction{second.y<first.y?-1:1};
  auto const pixel{[&]{if(region.contains(first.x,first.y))target.pixel(first.x,first.y,colour);}};
  if(dx>=dy){
    int error{2*dy-dx};
    for(int i{};i<=dx;++i){
      pixel();
      if(error>=0){first.y+=direction;error-=2*dx;}
      ++first.x;error+=2*dy;
    }
  }else{
    int error{2*dx-dy};
    for(int i{};i<=dy;++i){
      pixel();
      if(error>=0){++first.x;error-=2*dy;}
      first.y+=direction;error+=2*dx;
    }
  }
}
} // namespace

void draw_model_line(framebuffer &target, vec2<int32_t> first, vec2<int32_t> second, rectangle const bounds, uint8_t const colour, line_raster_mode const mode) {
  if(auto const *mask{target.clip()};mask&&mode==line_raster_mode::clip_regions){
    for(auto const &region:mask->regions)draw_region_line(target,vec2<int32_t>{first},vec2<int32_t>{second},bounds,colour,region);
    return;
  }
  if(!clip_line_rectangle(first, second, bounds)) return;
  if(second.x < first.x) std::swap(first, second);
  auto const dx{second.x - first.x};
  auto const dy{std::abs(second.y - first.y)};
  int const direction{second.y < first.y ? -1 : 1};
  int32_t error{0};
  if(dx > dy) {
    for(int32_t index{0}; index <= dx; ++index) {
      target.pixel(first.x + index, first.y, colour);
      error += dy + 1;
      if(error >= dx + 1) {
        error -= dx + 1;
        first.y += direction;
      }
    }
  } else {
    for(int32_t index{0}; index <= dy; ++index) {
      target.pixel(first.x, first.y + index * direction, colour);
      error += dx + 1;
      if(error >= dy + 1) {
        error -= dy + 1;
        ++first.x;
      }
    }
  }
}

} // namespace earthsiege
