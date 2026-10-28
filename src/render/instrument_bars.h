#pragma once
#include <array>
#include <optional>
#include "render/framebuffer.h"
#include "resources/cockpit.h"

namespace earthsiege {

struct led_bar_style {
  rectangle bounds;
  bool vertical{false};
  std::array<uint8_t, 2> colours{};          // alternating even/odd screen columns or rows
  uint8_t empty_colour{63};
};

/// Original full-paint LED gauge, with values expressed in 1/1024 of capacity.
void draw_led_bar(framebuffer &target, led_bar_style const &style, int32_t value);

class reactor_gauge {
  led_bar_style style;
  std::optional<screen_mask> cutout;
public:
  reactor_gauge(resource_store const &resources, cockpit_layout const &layout);
  void draw(framebuffer &target, int32_t value, int offset_x = 0, int offset_y = 0) const;
};

} // namespace earthsiege
