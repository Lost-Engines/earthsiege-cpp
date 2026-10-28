#pragma once
#include <string>
#include "resources/cockpit.h"

namespace earthsiege {

/// Original cockpit clock: a blinking colon, with one second per lit phase.
class clock_instrument {
  rectangle bounds;
  font face;
  uint32_t deadline;
  unsigned int seconds{};
  bool colon{};
public:
  clock_instrument(resource_store const &resources, cockpit_layout const &layout, uint32_t now = 0);
  void refresh();
  void update(uint32_t now);
  std::string text() const;
  bool colon_visible() const;
  uint32_t next_update() const;
  void draw(framebuffer &target, int offset_x = 0, int offset_y = 0) const;
};

} // namespace earthsiege
