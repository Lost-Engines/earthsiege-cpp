#pragma once
#include <array>
#include <optional>
#include "render/framebuffer.h"
#include "render/slider_geometry.h"
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

/// Original speed LEDs and requested-throttle thumb. Both values use Q10 units.
class throttle_gauge {
  bool vertical;
  rectangle slider;
  std::array<rectangle, 2> bars;
  std::array<bitmap, 3> lamps;
  bitmap thumb;
  int thumb_extent;
  std::array<bitmap, 2> painted_lamps;
  std::array<int, 2> lamp_boundaries{};
  bool lamps_initialized{};
  int32_t last_painted_speed{};
  void update_lamps(int32_t speed);
  void update_lamp(size_t index, int32_t speed);
public:
  throttle_gauge(resource_store const &resources, cockpit_layout const &layout);
  slider_geometry slider_layout() const;
  void repaint_lamps();
  void draw_retained_at(framebuffer &target, int32_t speed, rectangle const &thumb_position, int offset_x = 0, int offset_y = 0);
  rectangle thumb_bounds(int32_t requested) const;
  void draw_at(framebuffer &target, int32_t speed, rectangle const &thumb_position, int offset_x = 0, int offset_y = 0) const;
  void draw(framebuffer &target, int32_t speed, int32_t requested, int offset_x = 0, int offset_y = 0) const;
};

} // namespace earthsiege
