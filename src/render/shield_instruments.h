#pragma once
#include <array>
#include "resources/cockpit.h"

namespace earthsiege {

/// Six palette slots embedded in the cockpit artwork show front/rear charge.
void update_shield_colours(palette &colours, int32_t front, int32_t rear);

class shield_instruments {
  std::array<rectangle, 2> buttons;
  std::array<rectangle, 2> labels;
  std::array<bitmap, 6> faces;
  font numbers;
public:
  shield_instruments(resource_store const &resources, cockpit_layout const &layout);
  /// Balance is Q10; the original numbers sum to 200 percent.
  void draw(framebuffer &target, int32_t balance, std::array<unsigned int, 2> button_faces = {}, int offset_x = 0, int offset_y = 0) const;
};

} // namespace earthsiege
