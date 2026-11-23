#include "shield_instruments.h"
#include <algorithm>
#include <string>

namespace earthsiege {
namespace {
using shield_ramp = std::array<std::array<uint8_t, 3>, 3>;
constexpr std::array<int, 3> shield_green{25, 59, 23};

shield_ramp retail_ramp(int32_t charge) {
  // Retail 0x39340 subtracts a separate amount from each band's RGB components.
  int const percent{charge * 100 >> 9};
  std::array<int, 3> const dimming{
    percent > 125 ? 0 : percent < 25 ? 50 : 125 - percent,
    percent > 150 ? 0 : percent < 50 ? 50 : (150 - percent) >> 1,
    (200 - percent) >> 2,
  };
  shield_ramp ramp;
  for(size_t band{0}; band < ramp.size(); ++band) {
    for(size_t channel{0}; channel < shield_green.size(); ++channel) {
      ramp[band][channel] = static_cast<uint8_t>(std::max(0, shield_green[channel] - dimming[band]));
    }
  }
  return ramp;
}

shield_ramp expansion_ramp(int32_t charge) {
  // Expansion 0x3d0f4 scales the same green with three Q10 brightness curves.
  std::array<int32_t, 3> const brightness{
    4 * (std::clamp(charge, 384, 640) - 384),
    2 * (std::clamp(charge, 256, 768) - 256),
    charge,
  };
  shield_ramp ramp;
  for(size_t band{0}; band < ramp.size(); ++band) {
    for(size_t channel{0}; channel < shield_green.size(); ++channel) {
      int const scaled{(shield_green[channel] * brightness[band] + 512) >> 10};
      ramp[band][channel] = static_cast<uint8_t>(std::max(0, scaled));
    }
  }
  return ramp;
}
}

void update_shield_colours(palette &colours, int32_t const front, int32_t const rear, game_edition edition) {
  std::array<int32_t, 2> const charge{front, rear};
  for(size_t side{0}; side < charge.size(); ++side) {
    auto const ramp{edition == game_edition::expansion ? expansion_ramp(charge[side]) : retail_ramp(charge[side])};
    for(size_t band{0}; band < ramp.size(); ++band) {
      size_t const index{80 + side * ramp.size() + band};
      colours.dac_colours[index] = ramp[band];
      colours.colours[index] = display_colour(ramp[band]);
    }
  }
}

shield_instruments::shield_instruments(resource_store const &resources, cockpit_layout const &layout)
  : numbers{read_font(resources.read("simvol0", "dfn/CPGREEN.DFN"))} {
  auto const parent{layout.bounds(0x268)};
  auto const absolute{[&](size_t offset){
    auto bounds{layout.bounds(offset)};
    bounds.left += parent.left;
    bounds.right += parent.left;
    bounds.top += parent.top;
    bounds.bottom += parent.top;
    return bounds;
  }};
  buttons = {absolute(0x278), absolute(0x288)};
  labels = {absolute(0x298), absolute(0x2a8)};
  auto const bank{layout.read_buttons(resources)};
  for(size_t i{0}; i < faces.size(); ++i) faces[i] = bank.at(58 + i);
}

std::array<rectangle, 2> const &shield_instruments::button_bounds() const {
  return buttons;
}

void shield_instruments::draw(framebuffer &target, int32_t const balance, std::array<unsigned int, 2> const button_faces, int const offset_x, int const offset_y) const {
  scoped_framebuffer_clip clip{target, nullptr};
  int const front{balance * 200 >> 10};
  std::array<int, 2> const percentages{front, 200 - front};
  for(size_t side{0}; side < labels.size(); ++side) {
    auto bounds{labels[side]};
    bounds.left += offset_x;
    bounds.right += offset_x;
    bounds.top += offset_y;
    bounds.bottom += offset_y;
    auto const text{std::to_string(percentages[side])};
    int const x{bounds.left + ((bounds.right - bounds.left) >> 1) - ((numbers.text_width(text) - 1) >> 1)};
    int const y{bounds.top + ((bounds.bottom - bounds.top + static_cast<int>(numbers.height)) >> 1)};
    target.fill(bounds, 63);
    target.text(numbers, text, x, y);
  }
  for(size_t side{0}; side < buttons.size(); ++side) {
    target.image(faces.at(side * 3 + button_faces[side]), buttons[side].left + offset_x, buttons[side].top + offset_y, true);
  }
}

} // namespace earthsiege
