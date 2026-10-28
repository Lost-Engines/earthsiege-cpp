#include "mfd_instruments.h"
#include <stdexcept>
#include <utility>

namespace earthsiege {
namespace {
rectangle translate(rectangle value, int const x, int const y) {
  value.left += x; value.right += x; value.top += y; value.bottom += y;
  return value;
}
void label(framebuffer &target, font const &face, std::string_view const text, rectangle const &bounds) {
  int const x{bounds.left + ((bounds.right - bounds.left) >> 1) - ((face.text_width(text) - 1) >> 1)};
  int const y{bounds.top + ((bounds.bottom - bounds.top + static_cast<int>(face.height)) >> 1)};
  target.text(face, text, x, y);
}
}
std::array<bool, mfd_button_count> mfd_enabled_buttons(mfd_page const page) {
  std::array<bool, mfd_button_count> result{true, true, true, true, true, true};
  // Original 0x3a9ec reads seven flags per page at a thirteen-byte stride.
  // The trailing six bytes of each stored row are not button 0..5 flags.
  std::array<std::array<bool, 7>, 6> constexpr enabled{{
    {true, false, false, true, false, false, false},
    {true, false, false, true, true, true, true},
    {true, false, false, true, false, false, false},
    {true, true, true, true, false, false, false},
    {true, false, false, true, false, false, false},
    {true, false, false, true, false, false, false}
  }};
  auto const &row{enabled.at(std::to_underlying(page))};
  for(size_t i{}; i < row.size(); ++i) result[i + 6] = row[i];
  return result;
}
mfd_page next_mfd_page(mfd_page const page) {
  return static_cast<mfd_page>((std::to_underlying(page) + 1) % 6);
}
size_t next_mfd_range(size_t const current) {
  if(current >= mfd_ranges.size()) throw std::out_of_range{"MFD range index"};
  return (current + 1) % mfd_ranges.size();
}

mfd_instruments::mfd_instruments(resource_store const &resources, cockpit_layout const &source)
  : layout{source}, buttons{read_bitmaps(resources.read("simvol0", "dba/PBUTTONS.DBA"))},
    caption{read_font(resources.read("simvol0", "dfn/CPRED.DFN"))},
    button_fonts{read_font(resources.read("simvol0", "dfn/CPON.DFN")),
                 read_font(resources.read("simvol0", "dfn/CPPRESS.DFN")),
                 read_font(resources.read("simvol0", "dfn/CPOFF.DFN"))},
    labels{read_interface_strings(resources.read("simvol0", "str/STRINGS0.STR")).at(5)} {}
std::array<rectangle, mfd_button_count> mfd_instruments::button_bounds() const {
  auto const parent{layout.bounds(0x2d8)};
  std::array<rectangle, mfd_button_count> result;
  for(size_t i{}; i < result.size(); ++i) result[i] = translate(layout.bounds(0x2e8 + i * 16), parent.left, parent.top);
  return result;
}
rectangle mfd_instruments::display_bounds() const {
  auto const parent{layout.bounds(0x2d8)};
  return translate(layout.bounds(0x3b8), parent.left, parent.top);
}
void mfd_instruments::draw(framebuffer &target, mfd_page const page,
                           std::array<unsigned int, mfd_button_count> const &faces, int const offset_x, int const offset_y) const {
  scoped_framebuffer_clip clip{target, nullptr};
  auto const bounds{button_bounds()};
  for(size_t i{}; i < bounds.size(); ++i) {
    auto const box{translate(bounds[i], offset_x, offset_y)};
    if(i == 6) {
      target.fill(box, 63);
      label(target, caption, labels.at(std::to_underlying(page)).text, box);
    } else {
      auto const first{i < 6 ? 10 + 2 * i : i < 11 ? 3 : i == 11 ? 29 : 32};
      target.image(buttons.at(first + faces[i]), box.left, box.top, true);
      if(i > 6 && i < 11) label(target, button_fonts.at(faces[i]), labels.at(i).text, box);
    }
  }
}
} // namespace earthsiege
