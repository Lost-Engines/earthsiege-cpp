#pragma once
#include "resources/cockpit.h"
#include "resources/interface_strings.h"

namespace earthsiege {

enum class mfd_page : uint8_t { status, communications, navigation, scanner, target, missile_camera };
inline size_t constexpr mfd_button_count{13};
inline std::array<int32_t, 3> constexpr mfd_ranges{30000, 80000, 120000};
std::array<bool, mfd_button_count> mfd_enabled_buttons(mfd_page page);
mfd_page next_mfd_page(mfd_page page);
size_t next_mfd_range(size_t current);

/// Original MFD control strip, shared by its six pages.
class mfd_instruments {
  cockpit_layout layout;
  std::vector<bitmap> buttons;
  font caption;
  std::array<font, 3> button_fonts;
  std::vector<interface_string> labels;
public:
  mfd_instruments(resource_store const &resources, cockpit_layout const &source);
  std::array<rectangle, mfd_button_count> button_bounds() const;
  rectangle display_bounds() const;
  void draw(framebuffer &target, mfd_page page, std::array<unsigned int, mfd_button_count> const &faces,
            int offset_x = 0, int offset_y = 0) const;
};

} // namespace earthsiege
