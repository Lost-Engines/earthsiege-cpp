#include "instrument_bars.h"
#include <algorithm>
#include <stdexcept>
#include "simulation/fixed_math.h"

namespace earthsiege {

void draw_led_bar(framebuffer &target, led_bar_style const &style, int32_t const value) {
  auto const &bounds{style.bounds};
  auto const scale{wrap_long(static_cast<int64_t>(style.vertical ? bounds.bottom - bounds.top : bounds.right - bounds.left) * 65536) / 1024};
  auto const extent{wrap_long(static_cast<int64_t>(value) * scale) >> 16};
  if(style.vertical) {
    int const boundary{bounds.bottom - extent};
    target.fill({bounds.left, bounds.top, bounds.right, boundary}, style.empty_colour);
    for(int y{boundary + 1}; y <= bounds.bottom; ++y)
      target.fill({bounds.left, y, bounds.right, y}, style.colours[static_cast<unsigned int>(y) & 1]);
  } else {
    int const boundary{bounds.left + extent};
    for(int x{bounds.left}; x <= boundary; ++x)
      target.fill({x, bounds.top, x, bounds.bottom}, style.colours[static_cast<unsigned int>(x) & 1]);
    target.fill({boundary + 1, bounds.top, bounds.right, bounds.bottom}, style.empty_colour);
  }
}

reactor_gauge::reactor_gauge(resource_store const &resources, cockpit_layout const &layout) {
  auto const parent{layout.bounds(0x224)};
  auto bounds{layout.bounds(0x234)};
  bounds.left += parent.left; bounds.right += parent.left;
  bounds.top += parent.top; bounds.bottom += parent.top;
  auto const kind{layout.words[0x244 / 4]};
  if(kind != 3 && kind != 4) throw std::runtime_error{"unknown reactor bar type"};
  style = {.bounds{bounds}, .vertical{kind == 4}, .colours{static_cast<uint8_t>(layout.words[0x24c / 4]), static_cast<uint8_t>(layout.words[0x250 / 4])}};
  auto const mask{layout.words[0x248 / 4]};
  if(mask) {
    if(mask < 1 || mask > 4) throw std::runtime_error{"unknown reactor cutout"};
    cutout = read_screen_mask(resources.read("simvol0", "edg/ENG_CLP" + std::to_string(mask) + ".EDG"));
    cutout->translate(bounds.left, bounds.top);
  }
}

void reactor_gauge::draw(framebuffer &target, int32_t const value, int const offset_x, int const offset_y) const {
  auto translated{style};
  translated.bounds.left += offset_x; translated.bounds.right += offset_x;
  translated.bounds.top += offset_y; translated.bounds.bottom += offset_y;
  if(translated.bounds.right < 0 || translated.bounds.bottom < 0 || translated.bounds.left >= static_cast<int>(screen_width) || translated.bounds.top >= static_cast<int>(screen_height)) return;
  auto mask{cutout};
  if(mask) mask->translate(offset_x, offset_y);
  scoped_framebuffer_clip clip{target, mask ? &*mask : nullptr};
  draw_led_bar(target, translated, value);
}

namespace {
rectangle translated(rectangle bounds, int const x, int const y) {
  bounds.left += x; bounds.right += x;
  bounds.top += y; bounds.bottom += y;
  return bounds;
}

void crop_bitmap(framebuffer &target, bitmap const &source, rectangle const &whole, rectangle const &part) {
  for(int y{part.top}; y <= part.bottom; ++y) for(int x{part.left}; x <= part.right; ++x) {
    unsigned int const sx{static_cast<unsigned int>(x - whole.left)}, sy{static_cast<unsigned int>(y - whole.top)};
    if(sx < source.width && sy < source.height) target.pixel(x, y, source.pixels[sy * source.width + sx]);
  }
}

int bar_boundary(rectangle const &bounds, bool vertical, int32_t value, int32_t capacity) {
  int const first{vertical ? bounds.top : bounds.left}, last{vertical ? bounds.bottom : bounds.right};
  int32_t const scale{(last - first) * 65536 / capacity};
  int const extent{wrap_long(static_cast<int64_t>(value) * scale) >> 16};
  return std::clamp(vertical == (capacity > 0) ? last - extent : first + extent, first, last);
}

// The native widget paints the complete lit bitmap, then replaces the unlit
// interval inclusively. Retain that shared boundary pixel and fixed-point scale.
void bitmap_bar(framebuffer &target, rectangle const &bounds, bool const vertical,
                bitmap const &lit, bitmap const &unlit, int32_t const value, int32_t const capacity) {
  int const first{vertical ? bounds.top : bounds.left}, last{vertical ? bounds.bottom : bounds.right};
  bool const from_end{vertical == (capacity > 0)};
  int const boundary{bar_boundary(bounds,vertical,value,capacity)};
  crop_bitmap(target, lit, bounds, bounds);
  if(boundary == (from_end ? first : last)) return;
  auto empty{bounds};
  if(vertical) { if(from_end) empty.bottom = boundary; else empty.top = boundary; }
  else { if(from_end) empty.right = boundary; else empty.left = boundary; }
  crop_bitmap(target, unlit, bounds, empty);
}
} // namespace

throttle_gauge::throttle_gauge(resource_store const &resources, cockpit_layout const &layout)
  : vertical{layout.words[0x428 / 4] == 1}, thumb_extent{} {
  auto const parent{layout.bounds(0x3e8)};
  auto const kind{layout.words[0x428 / 4]};
  if(kind != 1 && kind != 2) throw std::runtime_error{"unknown throttle type"};
  slider = translated(layout.bounds(0x3f8), parent.left, parent.top);
  bars = {translated(layout.bounds(0x408), parent.left, parent.top), translated(layout.bounds(0x418), parent.left, parent.top)};
  auto const images{read_bitmaps(resources.read("simvol0", "dba/THR_LED" + std::to_string(kind) + ".DBA"))};
  if(images.size() != lamps.size()) throw std::runtime_error{"unexpected throttle lamp bank"};
  std::ranges::copy(images, lamps.begin());
  thumb = read_bitmaps(resources.read("simvol0", "dba/PBUTTONS.DBA")).at(static_cast<size_t>(layout.words[0x42c / 4]));
  // The horizontal widget reserves twelve pixels even though its button bitmap
  // is wider. This is an explicit native slider setting, not the sprite width.
  thumb_extent = vertical ? static_cast<int>(thumb.height) : 12;
}

slider_geometry throttle_gauge::slider_layout() const { return {slider, vertical, thumb_extent, -1024, 1024}; }
rectangle throttle_gauge::thumb_bounds(int32_t const requested) const {
  auto const layout{slider_layout()};
  return layout.thumb_at(layout.boundary_for_value(requested));
}

void throttle_gauge::draw(framebuffer &target, int32_t const speed, int32_t const requested, int const offset_x, int const offset_y) const {
  draw_at(target, speed, thumb_bounds(requested), offset_x, offset_y);
}

void throttle_gauge::update_lamp(size_t i, int32_t speed) {
  auto const &bounds{bars[i]};
  int const capacity{i==0?1024:-1024};
  int const boundary{bar_boundary(bounds,vertical,speed,capacity)};
  int &previous{lamp_boundaries[i]};
  if(boundary==previous)return;
  bool const from_end{vertical==(capacity>0)};
  bool const lit{from_end?boundary<previous:boundary>previous};
  auto const &source{lamps[lit?1:(i==0?0:2)]};
  auto &image{painted_lamps[i]};
  auto changed{bounds};
  if(vertical){changed.top=std::min(boundary,previous);changed.bottom=std::max(boundary,previous);}
  else {changed.left=std::min(boundary,previous);changed.right=std::max(boundary,previous);}
  // Original bar setters paint the old-to-new interval inclusively.
  for(int y{changed.top};y<=changed.bottom;++y)for(int x{changed.left};x<=changed.right;++x){
    auto sx{static_cast<unsigned>(x-bounds.left)},sy{static_cast<unsigned>(y-bounds.top)};
    if(sx<image.width&&sy<image.height&&sx<source.width&&sy<source.height)
      image.pixels[sy*image.width+sx]=source.pixels[sy*source.width+sx];
  }
  previous=boundary;
}
void throttle_gauge::repaint_lamps() {
  if(!lamps_initialized){
    for(size_t i{};i<bars.size();++i){
      painted_lamps[i]=lamps[i==0?0:2];
      bool const from_end{vertical==(i==0)};
      lamp_boundaries[i]=vertical?(from_end?bars[i].bottom:bars[i].top):(from_end?bars[i].right:bars[i].left);
    }
    lamps_initialized=true;
  }
  for(size_t i{};i<bars.size();++i){
    // Horizontal full painter 3bda8 resets the forward cache twice instead
    // of resetting the reverse one. Preserve both its cache and its pixels.
    size_t const reset{vertical?i:0};
    bool const from_end{vertical==(reset==0)};
    lamp_boundaries[reset]=vertical?(from_end?bars[reset].bottom:bars[reset].top):(from_end?bars[reset].right:bars[reset].left);
    update_lamp(i,i==0?1024:-1024);
    update_lamp(i,last_painted_speed);
  }
}
void throttle_gauge::update_lamps(int32_t speed) {
  if(!lamps_initialized)repaint_lamps();
  if(speed==last_painted_speed)return;
  for(size_t i{};i<bars.size();++i)update_lamp(i,speed);
  last_painted_speed=speed;
}
void throttle_gauge::draw_retained_at(framebuffer &target,int32_t speed,rectangle const &position,int offset_x,int offset_y) {
  update_lamps(speed);
  scoped_framebuffer_clip clip{target,nullptr};
  for(size_t i{};i<bars.size();++i){
    auto const bounds{translated(bars[i],offset_x,offset_y)};
    crop_bitmap(target,painted_lamps[i],bounds,bounds);
  }
  target.image(thumb,position.left+offset_x,position.top+offset_y,true);
}

void throttle_gauge::draw_at(framebuffer &target, int32_t const speed, rectangle const &position, int const offset_x, int const offset_y) const {
  scoped_framebuffer_clip clip{target, nullptr};
  for(size_t i{}; i < bars.size(); ++i)
    bitmap_bar(target, translated(bars[i], offset_x, offset_y), vertical, lamps[1], lamps[i == 0 ? 0 : 2], speed, i == 0 ? 1024 : -1024);
  target.image(thumb, position.left + offset_x, position.top + offset_y, true);
}

} // namespace earthsiege
