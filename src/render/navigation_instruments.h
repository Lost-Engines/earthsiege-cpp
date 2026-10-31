#pragma once
#include <array>
#include "resources/cockpit.h"

namespace earthsiege {

/// Original heading tape and incremental torso/elevation instruments.
class navigation_instruments {
  rectangle heading_bounds;
  rectangle torso_bounds;
  rectangle pitch_bounds;
  std::array<int, 2> cross;
  std::vector<bitmap> heading_art;
  std::vector<bitmap> torso_art;
  std::vector<bitmap> pitch_art;
  font tracking_font;
  std::string tracking_label;
  int16_t previous_heading{};
  int16_t previous_torso{};
  int16_t previous_pitch{};
  int16_t torso_indicator{};
  int16_t pitch_indicator{};
  size_t heading_first;
  size_t heading_second{0};
  int heading_x;

public:
  navigation_instruments(resource_store const &resources, cockpit_layout const &layout);
  void update(int16_t heading, int16_t torso, int16_t pitch, bool disabled = false);
  std::array<int16_t, 2> indicator_angles() const;
  uint16_t display_heading() const;
  void draw(framebuffer &target, int offset_x = 0, int offset_y = 0, bool tracking = false) const;
  void draw_tracking(framebuffer &target, int offset_x = 0, int offset_y = 0) const;
};

} // namespace earthsiege
