#pragma once
#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>
#include "render/framebuffer.h"
#include "resources/artwork.h"

namespace earthsiege {

class resource_store;

struct control {
  rectangle bounds;
  uint8_t type{};
  uint16_t flags{};
  uint16_t action{};
  std::string label;
};

struct button_style {
  uint8_t fill{};
  std::array<std::array<uint8_t, 4>, 3> borders{};
  std::array<uint16_t, 4> corners{};
  int corner_width{};
  int corner_height{};
};

class dialog {
  bitmap backdrop;
  bitmap panel;
  std::vector<bitmap> corners;
  std::vector<control> controls;
  std::array<button_style, 4> styles;
  font title_face;
  std::array<font, 3> button_faces;
  std::optional<unsigned int> pressed;
  palette colours;
  unsigned int number{};

public:
  explicit dialog(resource_store const &resources, unsigned int number = 1000);
  void draw(framebuffer &frame) const;
  void mouse_down(int x, int y);
  auto mouse_up(int x, int y)->std::optional<uint16_t>;
  void cancel_press();
  auto current_palette() const->palette const &;
  auto id() const->unsigned int;
};

} // namespace earthsiege
