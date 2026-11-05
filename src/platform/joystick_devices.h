#pragma once

#include <array>
#include <functional>
#include <vector>
#include "simulation/joystick_input.h"

namespace earthsiege {

struct host_joystick {
  int id{};
  std::vector<float> axes{};
  std::vector<uint8_t> buttons{};
  std::vector<uint8_t> hats{};
};
using joystick_source = std::function<std::vector<host_joystick>()>;

/// GLFW supplies host samples; the original calibration and response stay in core.
class joystick_devices {
  joystick_source source;
  std::array<int, 2> ids{-1, -1};
  std::vector<host_joystick> samples;
  auto stick(size_t slot) const->host_joystick const *;
  void refresh();

public:
  explicit joystick_devices(joystick_source source = {});
  auto poll(uint8_t model, bool pedals)->std::array<joystick_hardware, 2>;
  auto throttle() const->int16_t;
};

} // namespace earthsiege
