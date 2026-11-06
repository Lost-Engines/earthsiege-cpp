#pragma once
#include "simulation/joystick_input.h"
namespace earthsiege {
struct device_routing_options {
 uint8_t model{},mode{};
 bool pedals{},wcs{},external{},missile{},tilt_reverse{},backturn_reverse{},missile_reverse{},switch_held{},pedals_reverse{};
};
struct device_input_sample {
 std::array<int16_t,4> axes{};
 std::array<uint8_t,4> buttons{},hat{};
 bool aiming_camera{};
};
/// DBSIM 0x4cb14 after keyboard accumulation and electrical polling.
device_input_sample route_devices(device_routing_options &,joystick_sample keyboard,joystick_sample device,int16_t throttle);
}
