#include "dialog_widgets.h"
namespace earthsiege {
void draw_dialog_button(framebuffer &frame, rectangle const &bounds, button_style const &style, std::vector<bitmap> const &corners) {
  /// GO 0x44b86: centre fill, three four-sided borders, then sprite corners.
  frame.fill(rectangle{
    .left{bounds.left + 3},
    .top{bounds.top + 3},
    .right{bounds.right - 3},
    .bottom{bounds.bottom - 3},
  }, style.fill);
  for(unsigned int index{}; index < style.borders.size(); ++index) {
    int const inset{static_cast<int>(index)};
    auto const &colour{style.borders[index]};
    int const left{bounds.left + inset};
    int const right{bounds.right - inset};
    int const top{bounds.top + inset};
    int const bottom{bounds.bottom - inset};
    if(colour[0]) frame.line(left, top + 1, left, bottom - 1, colour[0]);
    if(colour[1]) frame.line(left + 1, top, right - 1, top, colour[1]);
    if(colour[2]) frame.line(right, top + 1, right, bottom - 1, colour[2]);
    if(colour[3]) frame.line(left + 1, bottom, right - 1, bottom, colour[3]);
  }
  frame.image(corners.at(style.corners[0]), bounds.left, bounds.top, true);
  frame.image(corners.at(style.corners[1]), bounds.right - style.corner_width, bounds.top, true);
  frame.image(corners.at(style.corners[2]), bounds.left, bounds.bottom - style.corner_height, true);
  frame.image(corners.at(style.corners[3]), bounds.right - style.corner_width, bounds.bottom - style.corner_height, true);
}

std::array<button_style,4> read_button_styles(bytes source){
  std::array<button_style,4> styles;
  binary_reader appearance{source, "button styles"};
  if(appearance.integer<uint16_t>() != styles.size()) appearance.fail("unexpected shell style count");
  for(auto &style : styles) {
    style.fill = static_cast<uint8_t>(appearance.integer<uint32_t>());
    for(auto &border : style.borders) {
      for(auto &colour : border) colour = static_cast<uint8_t>(appearance.integer<uint32_t>());
    }
    for(auto &corner : style.corners) corner = appearance.integer<uint16_t>();
    style.corner_width = appearance.integer<int16_t>();
    style.corner_height = appearance.integer<int16_t>();
  }
  appearance.require_end();
  return styles;
}
}
