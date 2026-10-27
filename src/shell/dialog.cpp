#include "dialog.h"
#include <algorithm>
#include <stdexcept>
#include "resources/resource_store.h"

namespace earthsiege {
namespace {

auto dialog_strings(bytes source)->std::vector<std::string> {
  binary_reader file{source, "dialog text"};
  auto const count{file.integer<uint32_t>()};
  auto const amount{file.integer<uint32_t>()};
  if(count > 1000) file.fail("excessive dialog strings");
  std::vector<unsigned int> offsets;
  for(unsigned int index{}; index < count; ++index) offsets.push_back(file.integer<uint16_t>());
  auto const pool{file.take(amount)};
  file.require_end();
  std::vector<std::string> result;
  for(auto const offset : offsets) {
    binary_reader text{pool, "dialog string"};
    text.seek(offset);
    result.push_back(text.cstring());
  }
  return result;
}

void draw_button(framebuffer &frame, rectangle const &bounds, button_style const &style, std::vector<bitmap> const &corners) {
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

} // namespace

dialog::dialog(resource_store const &resources, unsigned int requested) :
  backdrop{read_bitmaps(resources.read("shell0", "res32/regn1000.dbm")).front()},
  panel{read_bitmaps(resources.read("shell0", "res32/menu" + std::to_string(requested) + ".dba")).front()},
  corners{read_bitmaps(resources.read("shell0", "res32/corners.dba"))},
  title_face{read_font(resources.read("shell0", "dfn/title.dfn"))},
  button_faces{
    read_font(resources.read("shell0", "dfn/active.dfn")),
    read_font(resources.read("shell0", "dfn/pushed.dfn")),
    read_font(resources.read("shell0", "dfn/inactive.dfn")),
  },
  colours{read_palette(resources.read("shell0", "res32/regn1000.dpl"))},
  number{requested} {
  auto const labels{dialog_strings(resources.read("shell0", "eng/menu" + std::to_string(number) + ".bin"))};
  binary_reader file{resources.read("shell0", "dlg/menu" + std::to_string(number) + ".dlg"), "dialog controls"};
  auto const count{file.integer<uint16_t>()};
  if(!count || count > 1000) file.fail("invalid dialog control count");
  for(unsigned int index{}; index < count; ++index) {
    control entry{
      .bounds{
        .left{file.integer<int16_t>()},
        .top{file.integer<int16_t>()},
        .right{file.integer<int16_t>()},
        .bottom{file.integer<int16_t>()},
      },
      .type{file.integer<uint8_t>()},
      .flags{file.integer<uint16_t>()},
      .action{file.integer<uint16_t>()},
      .label{index < labels.size() ? labels[index] : ""},
    };
    if(index) {
      entry.bounds.left += controls.front().bounds.left;
      entry.bounds.right += controls.front().bounds.left;
      entry.bounds.top += controls.front().bounds.top;
      entry.bounds.bottom += controls.front().bounds.top;
    }
    controls.push_back(std::move(entry));
  }
  file.require_end();
  binary_reader appearance{resources.read("shell0", "cfg/styles.dat"), "shell button styles"};
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
}

void dialog::draw(framebuffer &frame) const {
  frame.image(backdrop, 0, 0);
  auto const &root{controls.front()};
  frame.image(panel, root.bounds.left, root.bounds.top, true);
  // GO 0x36b4c centres the caption inside a 40-pixel padded, 17-pixel plaque
  int const caption_width{title_face.text_width(root.label) + 40};
  int const caption_left{root.bounds.left + (root.bounds.right - root.bounds.left - caption_width) / 2};
  rectangle const caption{
    .left{caption_left},
    .top{root.bounds.top + 5},
    .right{caption_left + caption_width},
    .bottom{root.bounds.top + 22},
  };
  frame.line(caption.left, caption.top, caption.right, caption.top, 16);
  frame.line(caption.left, caption.top, caption.left, caption.bottom, 16);
  frame.line(caption.right, caption.top, caption.right, caption.bottom, 27);
  frame.line(caption.left, caption.bottom, caption.right, caption.bottom, 28);
  frame.image(corners.at(0), caption.left, caption.top, true);
  frame.image(corners.at(1), caption.right - 4, caption.top, true);
  frame.image(corners.at(2), caption.left, caption.bottom - 4, true);
  frame.image(corners.at(3), caption.right - 4, caption.bottom - 4, true);
  frame.text(title_face, root.label, caption.left + (caption_width - title_face.text_width(root.label)) / 2, caption.top + 7);
  for(unsigned int index{1}; index < controls.size(); ++index) {
    auto const &entry{controls[index]};
    if(entry.type == 15) draw_button(frame, entry.bounds, styles[3], corners);
    if(entry.type != 3) continue;
    bool const disabled{number == 1000 && entry.label == "RESUME"};
    unsigned int const state{disabled ? 2u : pressed == index ? 1u : 0u};
    draw_button(frame, entry.bounds, styles[state], corners);
    auto const &face{button_faces[state]};
    frame.text(face, entry.label, (entry.bounds.left + entry.bounds.right - face.text_width(entry.label)) / 2, entry.bounds.bottom - ((entry.bounds.bottom - entry.bounds.top) - static_cast<int>(face.height)) / 2 - 2 + (state == 1 ? 1 : 0));
  }
}

void dialog::mouse_down(int x, int y) {
  pressed.reset();
  for(unsigned int index{1}; index < controls.size(); ++index) {
    auto const &entry{controls[index]};
    if(entry.type == 3 && entry.bounds.contains(x, y) && !(number == 1000 && entry.label == "RESUME")) pressed = index;
  }
}

auto dialog::mouse_up(int x, int y)->std::optional<uint16_t> {
  auto const previous{pressed};
  pressed.reset();
  if(previous && controls[*previous].bounds.contains(x, y)) return controls[*previous].action;
  return {};
}

void dialog::cancel_press() { pressed.reset(); }
auto dialog::current_palette() const->palette const & { return colours; }
auto dialog::id() const->unsigned int { return number; }

} // namespace earthsiege
