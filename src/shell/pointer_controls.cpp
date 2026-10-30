#include "pointer_controls.h"
#include <stdexcept>

namespace earthsiege {

size_t pointer_controls::add_button(rectangle const bounds) {
  controls.push_back({.bounds{bounds}, .slider{}, .painted_thumb{}});
  return controls.size() - 1;
}
size_t pointer_controls::add_slider(slider_geometry const geometry, int const initial_value) {
  auto const index{add_button(geometry.bounds)};
  controls[index].slider = geometry;
  set_value(index, initial_value);
  painted(index);
  return index;
}
pointer_controls::control const &pointer_controls::at(size_t const index) const { return controls.at(index); }
std::optional<size_t> pointer_controls::active() const { return selected; }
bool pointer_controls::is_dragging() const { return dragging; }
bool pointer_controls::is_double_click() const { return double_click; }
void pointer_controls::window_bounds(rectangle const bounds) { window = bounds; }
void pointer_controls::enable(size_t const index, bool const enabled) { controls.at(index).face = enabled ? control_face::normal : control_face::disabled; }
void pointer_controls::set_value(size_t const index, int const value) {
  auto &item{controls.at(index)};
  if(!item.slider) throw std::logic_error{"value assigned to a button"};
  item.boundary = item.slider->boundary_for_value(value);
}
void pointer_controls::painted(size_t const index, std::optional<int> const boundary) {
  auto &item{controls.at(index)};
  if(item.slider) item.painted_thumb = item.slider->thumb_at(boundary.value_or(item.boundary));
}
void pointer_controls::reset_input() {
  selected.reset(); dragging = left_held = right_held = false;
  for(auto &item : controls) if(item.face == control_face::pressed) item.face = control_face::normal;
}
std::optional<size_t> pointer_controls::hit(int const x, int const y) const {
  for(size_t i{}; i < controls.size(); ++i)
    if(controls[i].face != control_face::disabled && controls[i].bounds.contains(x, y)) return i;
  return {};
}
std::optional<control_activation> pointer_controls::event(int const x, int const y, bool const left, bool const right, uint32_t const ticks) {
  double_click=false;
  if(x != last_x || y != last_y) {
    last_x = x; last_y = y;
    if(dragging && selected) {
      auto &item{controls[*selected]};
      if(item.slider) item.boundary = item.slider->clamp_boundary(x, y);
    } else if(selected && window.contains(x, y)) {
      auto &item{controls[*selected]};
      bool const over{hit(x, y) == selected};
      if(over && item.face == control_face::normal) item.face = control_face::pressed;
      else if(!over && item.face == control_face::pressed) item.face = control_face::normal;
    }
  }
  if(left && !left_held) {
    selected = hit(x, y);
    if(selected) {
      auto &item{controls[*selected]}; item.face = control_face::pressed;
      if(item.slider) { item.boundary = item.slider->clamp_boundary(x, y); dragging = true; }
    }
  }
  bool const release_left{left_held && !left}, release_right{right_held && !right};
  left_held = left; right_held = right;
  if(!release_left && !release_right) return {};
  // Original mouse timestamps use the 120 Hz clock. Releases of either button
  // within half a second set the shared latch, even at different positions.
  if(release_left){double_click=ticks-release_ticks[0]<60;release_ticks[0]=ticks;}
  if(release_right){double_click=double_click || ticks-release_ticks[1]<60;release_ticks[1]=ticks;}
  int const buttons{(left ? 1 : 0) | (right ? 2 : 0)};
  // Native handling overwrites the released-bit word for a right release.
  int const released_buttons{buttons | (release_right ? 2 : 1)};
  std::optional<control_activation> result;
  if(dragging && selected) {
    auto &item{controls[*selected]}; item.face = control_face::normal;
    result = control_activation{*selected, item.slider ? item.slider->axis_value(item.painted_thumb) : released_buttons,double_click};
    dragging = false;
  } else {
    auto const previous{selected}; selected = hit(x, y);
    if(selected) {
      auto &item{controls[*selected]};
      if(selected == previous || (released_buttons & 2))
        result = control_activation{*selected, item.slider ? item.slider->axis_value(item.painted_thumb) : released_buttons,double_click};
      item.face = control_face::normal;
    }
  }
  selected.reset();
  return result;
}

} // namespace earthsiege
