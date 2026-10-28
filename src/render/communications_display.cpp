#include "communications_display.h"

namespace earthsiege {
size_t communications_display_state::command() const { return selected + (choices.at(selected).alternate ? 3 : 0); }
communications_display::communications_display(resource_store const &resources)
  : fonts{read_font(resources.read("simvol0", "dfn/CPGREEN.DFN")),
          read_font(resources.read("simvol0", "dfn/CPYLW.DFN")),
          read_font(resources.read("simvol0", "dfn/CPRED.DFN")),
          read_font(resources.read("simvol0", "dfn/CPOFF.DFN"))},
    commands{read_interface_strings(resources.read("simvol0", "str/STRINGS0.STR")).at(0)} {}
std::array<rectangle, 6> communications_display::row_bounds(rectangle const &bounds) const {
  std::array<rectangle, 6> result;
  int const step{(bounds.bottom - bounds.top) / static_cast<int>(result.size())};
  for(size_t i{}; i < result.size(); ++i) {
    auto &row{result[i]}; row = bounds;
    row.top += static_cast<int>(i) * step;
    row.bottom = row.top + static_cast<int>(fonts[0].height);
  }
  return result;
}
void communications_display::draw(framebuffer &target, rectangle const &bounds, communications_display_state const &state) const {
  scoped_framebuffer_clip clip{target, nullptr};
  target.fill(bounds, 63);
  auto const rows{row_bounds(bounds)};
  for(size_t i{}; i < rows.size(); ++i) {
    auto const &choice{state.choices[i]};
    auto const &entry{commands.at(i + (choice.alternate ? 3 : 0))};
    auto const &face{fonts[choice.disabled ? 3 : i == state.selected ? 1 : 0]};
    auto const &row{rows[i]};
    int const x{row.left + 1}, y{row.top + ((row.bottom - row.top + static_cast<int>(face.height)) >> 1)};
    target.fill(row, 63);
    target.text(face, entry.text, x, y);
    if(!entry.attributes.empty() && entry.attributes[0] < entry.text.size()) {
      auto const index{entry.attributes[0]};
      int const hotkey_x{x + face.text_width(std::string_view{entry.text}.substr(0, index))};
      target.text(fonts[choice.disabled ? 3 : 2], std::string_view{entry.text}.substr(index, 1), hotkey_x, y);
    }
  }
}
} // namespace earthsiege
