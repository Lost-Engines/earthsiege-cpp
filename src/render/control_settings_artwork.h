#pragma once
#include "render/dialog_widgets.h"
#include "resources/interface_strings.h"
#include "resources/resource_store.h"
namespace earthsiege {
/// Original GO 0x695f CTL_ALRT window and value fields.
class control_settings_artwork {
  bitmap background;
  std::vector<bitmap> corners;
  std::array<button_style, 4> styles;
  font title;
  font metrics;
  font values;
  std::array<font, 3> faces;
  interface_strings strings;
  int origin_x{49};
  int origin_y{50};
  uint8_t field_background{31};
  int width{0};
  int title_padding{0};
  int title_top{0};
  int title_height{0};
  int title_offset{0};
  std::array<rectangle, 7> buttons;
  std::array<int, 7> offsets{};
  std::array<rectangle, 5> fields;
  int field_x{0};
  int field_y{0};
public:
  explicit control_settings_artwork(resource_store const &, bool in_flight = false);
  rectangle button(unsigned i) const {
    return buttons.at(i);
  }
  void draw(framebuffer &, std::span<uint8_t const, 5> state, std::span<uint8_t const, 7> appearance) const;
};
} // namespace earthsiege
