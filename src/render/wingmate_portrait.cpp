#include "wingmate_portrait.h"
#include <numeric>
#include "simulation/instrument_failure.h"

namespace earthsiege {
void wingmate_portrait::draw_empty(framebuffer &target, std::array<rectangle, 3> const &bounds, unsigned int occupied, game_edition const edition) {
  // Original 3cb90's active-portrait painter leaves a temporary brush pointer
  // in the context. Its next empty fill works, but overwrites that brush with
  // the rectangle, so subsequent empty fills do nothing. Preserve the visible
  // result for the three original slots without retaining an invalid pointer.
  bool temporary{false};
  bool valid{true};
  for(size_t slot{0}; slot < bounds.size(); ++slot) {
    if(occupied & (1u << slot)) {
      temporary = true;
      valid = true;
    } else if(valid) {
      target.fill(bounds[slot], 63);
      if(temporary && edition == game_edition::original) valid = false;
    }
  }
}

wingmate_portrait::wingmate_portrait(resource_store const &resources)
  : paint_margin{resources.edition() == game_edition::original ? 1 : 0},
    red{read_font(resources.read("simvol0", "dfn/CPRED.DFN"))},
    blue{read_font(resources.read("simvol0", "dfn/CPBLUE.DFN"))},
   strings{read_interface_strings(resources.read("simvol0", "str/STRINGS0.STR"))},
   names{read_interface_strings(resources.read("simvol0", "str/PILOTS.STR")).at(0)},
   noise{read_bitmaps(resources.read("simvol0", "dba/STATIC.DBA"))} {}
std::string const &wingmate_portrait::name(size_t const pilot) const {
  return names.at(pilot).text;
}
void wingmate_portrait::label(framebuffer &target, rectangle const b, font const &face, std::string_view const text, uint8_t const background) const {
  if(background) target.fill(b, background);
  int const x{b.left + ((b.right - b.left) >> 1) - ((face.text_width(text) - 1) >> 1)};
  int const y{b.top + ((b.bottom - b.top + static_cast<int>(face.height)) >> 1)};
  target.text(face, text, x, y);
}
void wingmate_portrait::draw_name(framebuffer &target, rectangle const portrait, size_t const pilot, size_t const slot) const {
  auto b{portrait.inset(-paint_margin)};
  b.left += 2;
  b.right -= 2;
  b.top += 4;
  b.bottom = b.top + static_cast<int>(red.height);
  label(target, b, red, name(pilot), std::array<uint8_t, 3>{56, 59, 49}.at(slot));
}
void wingmate_portrait::draw_idle(framebuffer &target, rectangle const portrait, size_t const pilot, size_t const slot, std::span<int16_t const, 19> const armour, int16_t const orders) const {
  scoped_framebuffer_clip clip{target, nullptr};
  target.fill(portrait, 63);
  draw_name(target, portrait, pilot, slot);
  auto const health{instrument_damage_band(static_cast<int16_t>(std::accumulate(armour.begin(), armour.end(), 0) / 19))};
  std::array<std::string_view, 4> const text{strings.at(29).at(0).text, strings.at(24).at(health).text, strings.at(29).at(1).text, strings.at(34).at(static_cast<size_t>(orders)).text};
  for(size_t i{0}; i < text.size(); ++i) {
    auto b{portrait.inset(-paint_margin)};
    b.top += 16 + static_cast<int>(i) * 8;
    b.bottom = b.top + static_cast<int>(red.height);
    label(target, b, i % 2 ? red : blue, text[i]);
  }
}
void wingmate_portrait::draw_static(framebuffer &target, rectangle const portrait, size_t const pilot, size_t const slot, size_t const frame) const {
  scoped_framebuffer_clip clip{target, nullptr};
  auto const &image{noise.at(frame)};
  for(unsigned int y{0}; y < image.height; ++y) {
    for(unsigned int x{0}; x < image.width; ++x) {
      int const px{portrait.left + static_cast<int>(x)};
      int const py{portrait.top + static_cast<int>(y)};
      auto const colour{image.pixels[y * image.width + x]};
      if(colour && portrait.contains(px, py)) target.pixel(px, py, colour);
    }
  }
  draw_name(target, portrait, pilot, slot);
}
} // namespace earthsiege
