#pragma once
#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>
#include "render/framebuffer.h"
#include "resources/artwork.h"
#include "render/dialog_widgets.h"

namespace earthsiege {

class resource_store;

struct control {
  rectangle bounds;
  uint8_t type{};
  uint8_t style{};
  uint8_t key_scan{};
  uint16_t action{};
  std::string label;
  bool enabled{true};
  std::vector<std::string> items;
  int selection{};
  int first_row{};
};


class dialog {
  bitmap backdrop;
  bitmap panel;
  std::vector<bitmap> corners;
  std::vector<control> controls;
  std::array<button_style, 4> styles;
  font title_face;
  font caption_face;
  unsigned region_number{};
  std::array<font, 3> button_faces;
  std::array<font, 2> list_faces;
  std::optional<unsigned int> pressed;
  palette colours;
  unsigned int number{};
  std::optional<unsigned int> text_focus;
  int row_height(control const &entry) const;
  void reveal(control &entry);

public:
  explicit dialog(resource_store const &resources, unsigned int number = 1000, unsigned int region = 1000);
  void draw(framebuffer &frame, bool background = true) const;
  control *find(uint16_t action);
  control const *find(uint16_t action) const;
  void enable(uint16_t action, bool enabled);
  void replace_text(std::string_view from,std::string_view to);
  void items(uint16_t action, std::vector<std::string> values, int selected = 0);
  void select(uint16_t action, int selected);
  int selection(uint16_t action) const;
  std::optional<uint16_t> shortcut(uint8_t scan) const;
  void text_input(std::string_view text);
  void backspace();
  void mouse_down(int x, int y);
  auto mouse_up(int x, int y)->std::optional<uint16_t>;
  void cancel_press();
  auto current_palette() const->palette const &;
  auto id() const->unsigned int;
  unsigned region()const{return region_number;}
};

} // namespace earthsiege
