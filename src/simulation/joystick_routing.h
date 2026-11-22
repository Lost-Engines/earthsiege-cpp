#pragma once
#include "simulation/joystick_input.h"
namespace earthsiege {
struct device_routing_options {
  uint8_t model{0};
  uint8_t mode{0};
  bool pedals{false};
  bool wcs{false};
  bool external{false};
  bool missile{false};
  bool tilt_reverse{false};
  bool backturn_reverse{false};
  bool missile_reverse{false};
  bool switch_held{false};
  bool pedals_reverse{false};
};
struct device_input_sample {
  std::array<int16_t, 4> axes{};
  std::array<uint8_t, 4> buttons{};
  std::array<uint8_t, 4> hat{};
  bool aiming_camera{false};
};
/// DBSIM 0x4cb14 after keyboard accumulation and electrical polling.
device_input_sample route_devices(device_routing_options &, joystick_sample keyboard, joystick_sample device, int16_t throttle);
} // namespace earthsiege
