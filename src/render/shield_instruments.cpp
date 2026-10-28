#include "shield_instruments.h"
#include <algorithm>
#include <string>

namespace earthsiege {

void update_shield_colours(palette &colours, int32_t const front, int32_t const rear) {
  // Original 0x39340 changes the DAC rather than redrawing the shield artwork.
  std::array<int, 3> constexpr base{25, 59, 23};
  std::array<int32_t, 2> const charge{front, rear};
  for(size_t side{}; side < charge.size(); ++side) {
    int const percent{charge[side] * 100 >> 9};
    std::array<int, 3> const dimming{
      percent > 125 ? 0 : percent < 25 ? 50 : 125 - percent,
      percent > 150 ? 0 : percent < 50 ? 50 : (150 - percent) >> 1,
      (200 - percent) >> 2,
    };
    for(size_t level{}; level < dimming.size(); ++level) {
      size_t const index{80 + side * 3 + level};
      auto &dac{colours.dac_colours[index]};
      for(size_t component{}; component < base.size(); ++component)
        dac[component] = static_cast<uint8_t>(std::max(0, base[component] - dimming[level]));
      colours.colours[index] = 0xff000000u | (static_cast<uint32_t>(dac[0] * 255 / 63) << 16)
                             | (static_cast<uint32_t>(dac[1] * 255 / 63) << 8) | static_cast<uint32_t>(dac[2] * 255 / 63);
    }
  }
}

shield_instruments::shield_instruments(resource_store const &resources, cockpit_layout const &layout)
  : numbers{read_font(resources.read("simvol0", "dfn/CPGREEN.DFN"))} {
  auto const parent{layout.bounds(0x268)};
  auto const absolute{[&](size_t offset){
    auto bounds{layout.bounds(offset)};
    bounds.left += parent.left; bounds.right += parent.left;
    bounds.top += parent.top; bounds.bottom += parent.top;
    return bounds;
  }};
  buttons = {absolute(0x278), absolute(0x288)};
  labels = {absolute(0x298), absolute(0x2a8)};
  auto const bank{read_bitmaps(resources.read("simvol0", "dba/PBUTTONS.DBA"))};
  for(size_t i{}; i < faces.size(); ++i) faces[i] = bank.at(58 + i);
}

void shield_instruments::draw(framebuffer &target, int32_t const balance, std::array<unsigned int, 2> const button_faces, int const offset_x, int const offset_y) const {
  scoped_framebuffer_clip clip{target, nullptr};
  int const front{balance * 200 >> 10};
  std::array<int, 2> const percentages{front, 200 - front};
  for(size_t side{}; side < labels.size(); ++side) {
    auto bounds{labels[side]};
    bounds.left += offset_x; bounds.right += offset_x;
    bounds.top += offset_y; bounds.bottom += offset_y;
    auto const text{std::to_string(percentages[side])};
    int const x{bounds.left + ((bounds.right - bounds.left) >> 1) - ((numbers.text_width(text) - 1) >> 1)};
    int const y{bounds.top + ((bounds.bottom - bounds.top + static_cast<int>(numbers.height)) >> 1)};
    target.fill(bounds, 63);
    target.text(numbers, text, x, y);
  }
  for(size_t side{}; side < buttons.size(); ++side)
    target.image(faces.at(side * 3 + button_faces[side]), buttons[side].left + offset_x, buttons[side].top + offset_y, true);
}

} // namespace earthsiege
