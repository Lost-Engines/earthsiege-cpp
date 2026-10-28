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

// The native widget paints the complete lit bitmap, then replaces the unlit
// interval inclusively. Retain that shared boundary pixel and fixed-point scale.
void bitmap_bar(framebuffer &target, rectangle const &bounds, bool const vertical,
                bitmap const &lit, bitmap const &unlit, int32_t const value, int32_t const capacity) {
  int const first{vertical ? bounds.top : bounds.left}, last{vertical ? bounds.bottom : bounds.right};
  int32_t const scale{(last - first) * 65536 / capacity};
  int const extent{wrap_long(static_cast<int64_t>(value) * scale) >> 16};
  bool const from_end{vertical == (capacity > 0)};
  int const boundary{std::clamp(from_end ? last - extent : first + extent, first, last)};
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

rectangle throttle_gauge::thumb_bounds(int32_t const requested) const {
  auto result{slider};
  int const travel{(vertical ? slider.bottom - slider.top : slider.right - slider.left) - thumb_extent};
  int const scale{travel * 65536 / 2048};
  int const extent{((std::clamp(requested, -1024, 1024) + 1024) * scale) >> 16};
  if(vertical) { result.bottom -= extent; result.top = result.bottom - thumb_extent; }
  else { result.left += extent; result.right = result.left + thumb_extent; }
  return result;
}

void throttle_gauge::draw(framebuffer &target, int32_t const speed, int32_t const requested, int const offset_x, int const offset_y) const {
  scoped_framebuffer_clip clip{target, nullptr};
  for(size_t i{}; i < bars.size(); ++i)
    bitmap_bar(target, translated(bars[i], offset_x, offset_y), vertical, lamps[1], lamps[i == 0 ? 0 : 2], speed, i == 0 ? 1024 : -1024);
  auto const position{thumb_bounds(requested)};
  target.image(thumb, position.left + offset_x, position.top + offset_y, true);
}

} // namespace earthsiege
