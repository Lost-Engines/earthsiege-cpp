#include "bitmap_raster.h"
#include <algorithm>

namespace earthsiege {

void draw_scaled_bitmap(framebuffer &target, bitmap const &source, vec2<int32_t> const position, vec2<int32_t> const size, rectangle const bounds) {
  if(size.x <= 0 || size.y <= 0) return;
  int32_t const left{std::max(bounds.left, position.x)};
  int32_t const top{std::max(bounds.top, position.y)};
  int32_t const right{std::min(bounds.right, position.x + size.x - 1)};
  int32_t const bottom{std::min(bounds.bottom, position.y + size.y - 1)};
  if(left > right || top > bottom) return;
  int32_t const x_step{static_cast<int32_t>((static_cast<int64_t>(source.width) << 16) / size.x) + 1};
  int32_t const y_step{static_cast<int32_t>((static_cast<int64_t>(source.height) << 16) / size.y) + 1};
  int64_t const skipped{top - position.y};
  int64_t const first_row{skipped * y_step >> 16};
  // Native top clipping includes the bottom crop in the initial fractional carry.
  int64_t const fraction{skipped ? (static_cast<int64_t>(size.y - (bottom - top + 1)) * y_step) & 65535 : 0};
  for(int32_t y{top}; y <= bottom; ++y) {
    int64_t const row{first_row + (fraction + static_cast<int64_t>(y - top) * y_step) / 65535};
    for(int32_t x{left}; x <= right; ++x) {
      int64_t const column{static_cast<int64_t>(x - position.x) * x_step >> 16};
      auto const colour{source.pixels.at(static_cast<size_t>(row * source.width + column))};
      if(colour) target.pixel(x, y, colour);
    }
  }
}

void draw_shell_scaled_bitmap(framebuffer &target,bitmap const &source,vec2<int32_t> position,vec2<int32_t> size,rectangle bounds){
  if(size.x<=0||size.y<=0)return;
  int const left{std::max(bounds.left,position.x)},top{std::max(bounds.top,position.y)};
  int const right{std::min(bounds.right,position.x+size.x-1)},bottom{std::min(bounds.bottom,position.y+size.y-1)};
  if(left>right||top>bottom)return;
  auto const step{[](int source_size,int destination_size){return destination_size>1?((source_size-1)<<16)/(destination_size-1):0;}};
  auto const fraction{[](int increment,int destination_size){return destination_size<=1?0:increment<32768?32768:increment&65535;}};
  int const dx{step(source.width,size.x)},dy{step(source.height,size.y)};
  int xf{fraction(dx,size.x)},yf{fraction(dy,size.y)};
  for(int y{top};y<=bottom;++y){
    auto const row{(static_cast<int64_t>(y-position.y)*dy+yf)>>16};
    // GO retains the fractional horizontal clipping carry between scanlines.
    auto const first{static_cast<int64_t>(left-position.x)*dx+xf};xf=static_cast<int>(first&65535);
    for(int x{left};x<=right;++x){
      auto const column{(first+static_cast<int64_t>(x-left)*dx)>>16};
      auto const colour{source.pixels.at(static_cast<size_t>(row*source.width+column))};
      if(colour)target.pixel(x,y,colour);
    }
  }
}

} // namespace earthsiege
