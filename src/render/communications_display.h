#pragma once
#include "resources/cockpit.h"
#include "resources/interface_strings.h"

namespace earthsiege {

struct communications_choice {
  bool disabled{};
  bool alternate{};
};
struct communications_display_state {
  size_t selected{};
  std::array<communications_choice, 6> choices{};
  size_t command() const;
};
class communications_display {
  std::array<font, 4> fonts;                                                // green, yellow, red, disabled
  std::vector<interface_string> commands;
public:
  explicit communications_display(resource_store const &resources);
  std::array<rectangle, 6> row_bounds(rectangle const &bounds) const;
  void draw(framebuffer &target, rectangle const &bounds, communications_display_state const &state) const;
};

} // namespace earthsiege
