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

} // namespace earthsiege
