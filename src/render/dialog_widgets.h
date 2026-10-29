#pragma once
#include "render/framebuffer.h"
#include "resources/artwork.h"
namespace earthsiege {
struct button_style {
  uint8_t fill{};
  std::array<std::array<uint8_t, 4>, 3> borders{};
  std::array<uint16_t, 4> corners{};
  int corner_width{};
  int corner_height{};
};

std::array<button_style,4> read_button_styles(bytes source);
void draw_dialog_button(framebuffer &frame,rectangle const &bounds,button_style const &style,std::vector<bitmap> const &corners);
}
