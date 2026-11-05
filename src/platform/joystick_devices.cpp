#include "joystick_devices.h"
#include <algorithm>
#include <cmath>
#include <utility>
#include <GLFW/glfw3.h>

namespace earthsiege {
namespace {
auto read_devices()->std::vector<host_joystick> {
  std::vector<host_joystick> result;
  for(int id{GLFW_JOYSTICK_1}; id <= GLFW_JOYSTICK_LAST; ++id) {
    if(!glfwJoystickPresent(id)) continue;
    host_joystick sample{.id = id};
    int count{};
    if(auto const *data{glfwGetJoystickAxes(id, &count)}) sample.axes.assign(data, data + count);
    if(auto const *data{glfwGetJoystickButtons(id, &count)}) sample.buttons.assign(data, data + count);
    if(auto const *data{glfwGetJoystickHats(id, &count)}) sample.hats.assign(data, data + count);
    result.push_back(std::move(sample));
  }
  return result;
}
auto axis(host_joystick const *s, size_t i)->int32_t {
  if(!s || i >= s->axes.size() || !std::isfinite(s->axes[i])) return 32768;
  float const value{std::clamp(s->axes[i], -1.0f, 1.0f)};
  return 32768 + static_cast<int32_t>(std::lround(value * (value < 0 ? 32768 : 32767)));
}
auto buttons(host_joystick const *s)->uint8_t {
  uint8_t result{};
  if(s) for(size_t i{}; i < 4 && i < s->buttons.size(); ++i) {
    if(s->buttons[i]) result |= static_cast<uint8_t>(1 << i);
  }
  return result;
}
} // namespace

joystick_devices::joystick_devices(joystick_source input) : source{input ? std::move(input) : read_devices} {}
auto joystick_devices::stick(size_t slot) const->host_joystick const * {
  auto const found{std::ranges::find(samples, ids[slot], &host_joystick::id)};
  return found == samples.end() ? nullptr : &*found;
}
void joystick_devices::refresh() {
  samples = source();
  for(size_t i{}; i < ids.size(); ++i) if(!stick(i)) ids[i] = -1;
  for(auto const &sample : samples) {
    if(std::ranges::find(ids, sample.id) != ids.end()) continue;
    auto const free{std::ranges::find(ids, -1)};
    if(free != ids.end()) *free = sample.id;
  }
}
auto joystick_devices::poll(uint8_t model, bool pedals)->std::array<joystick_hardware, 2> {
  refresh();
  std::array<joystick_hardware, 2> out;
  auto const *a{stick(0)};
  auto const *b{stick(1)};
  if(a) {
    out[0].present = true;
    out[0].buttons = buttons(a);
    for(size_t i{}; i < 4; ++i) out[0].axes[i] = axis(a, i);
    size_t const slider{a->axes.size() > 3 ? 3u : 2u};
    uint8_t const hat{a->hats.empty() ? uint8_t{0} : a->hats[0]};
    int const direction{hat & GLFW_HAT_UP ? 0 : hat & GLFW_HAT_RIGHT ? 1 : hat & GLFW_HAT_DOWN ? 2 : hat & GLFW_HAT_LEFT ? 3 : -1};
    if(model == 3) out[0].axes[3] = direction < 0 ? 32768 : direction == 0 ? 0 : direction == 1 ? 16384 : direction == 2 ? 8192 : 24576;
    if(model == 4 || model == 5) {
      out[0].axes[3] = a->axes.size() > slider ? axis(a, slider) / 2 : 16384;
      if(model == 5 && direction >= 0) out[0].buttons = static_cast<uint8_t>(direction == 0 ? 15 : direction == 1 ? 7 : direction == 2 ? 11 : 3);
    }
  }
  if(b) {
    out[1].present = true;
    out[1].axes[2] = axis(b, 0);
    out[1].axes[3] = axis(b, 1);
    out[1].buttons = static_cast<uint8_t>((buttons(b) & 3) << 2);
  } else if(a && ((model == 2 && a->axes.size() >= 4) || (pedals && a->axes.size() >= 3))) {
    out[1].present = true;
    out[1].axes[2] = axis(a, 2);
    out[1].axes[3] = axis(a, 3);
    out[1].buttons = static_cast<uint8_t>(buttons(a) & 12);
  }
  return out;
}
auto joystick_devices::throttle() const->int16_t {
  auto const *s{stick(0)};
  if(!s || s->axes.size() < 3) return 0;
  return static_cast<int16_t>(axis(s, s->axes.size() > 3 ? 3u : 2u) / 128 - 256);
}

} // namespace earthsiege
