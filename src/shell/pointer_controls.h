#pragma once
#include <optional>
#include <vector>
#include "render/slider_geometry.h"

namespace earthsiege {

enum class control_face : uint8_t { normal, pressed, disabled };
struct control_activation { size_t index; int value; bool double_click{}; };

/// Original cockpit mouse queue semantics, independent of the host window system.
/// Actions are returned to the owning instrument. Button values retain the mouse
/// bits (1 left, 2 right); slider values come from the last completed paint.
class pointer_controls {
public:
  struct control {
    rectangle bounds;
    control_face face{control_face::normal};
    std::optional<slider_geometry> slider;
    int boundary{};
    rectangle painted_thumb;
  };
private:
  std::vector<control> controls;
  rectangle window{0, 0, 319, 199};
  std::optional<size_t> selected;
  bool dragging{false};
  bool left_held{false}, right_held{false};
  int last_x{}, last_y{};
  std::array<uint32_t,2> release_ticks{};
  bool double_click{};
  std::optional<size_t> hit(int x, int y) const;
public:
  size_t add_button(rectangle bounds);
  size_t add_slider(slider_geometry geometry, int initial_value);
  control const &at(size_t index) const;
  std::optional<size_t> active() const;
  bool is_dragging() const;
  bool is_double_click() const;
  void window_bounds(rectangle bounds);
  void bounds(size_t index,rectangle value){controls.at(index).bounds=value;}
  void enable(size_t index, bool enabled);
  void set_value(size_t index, int value);
  void painted(size_t index, std::optional<int> boundary = {});
  void reset_input();
  std::optional<control_activation> event(int x, int y, bool left, bool right, uint32_t ticks = 0);
};

} // namespace earthsiege
