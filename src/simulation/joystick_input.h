#pragma once
#include <array>
#include <cstdint>
#include "resources/binary_reader.h"
namespace earthsiege {
struct joystick_calibration {
  std::array<int32_t, 4> minimum{};
  std::array<int32_t, 4> maximum{};
  std::array<int32_t, 4> centre_low{};
  std::array<int32_t, 4> centre_high{};
  std::array<int32_t, 16> values() const;
  static joystick_calibration from_values(std::array<int32_t, 16> const &);
  bool operator==(joystick_calibration const &) const = default;
};
joystick_calibration read_joystick_calibration(bytes);
std::vector<uint8_t> write_joystick_calibration(joystick_calibration const &);
enum class joystick_kind : uint8_t {
  standard,
  rudder,
  fcs,
  flightstick,
  flightstick_pro
};
struct joystick_hardware {
  std::array<int32_t, 4> axes{32768, 32768, 32768, 32768};
  uint8_t buttons{0};
  bool present{false};
};
struct joystick_profile {
  joystick_kind kind{joystick_kind::standard};
  uint8_t axis_mask{3};
  uint8_t button_mask{3};
  uint8_t response{1};
  uint8_t hat{0};
};
struct joystick_sample {
  std::array<int32_t, 4> axes{};
  std::array<uint8_t, 4> buttons{};
  std::array<uint8_t, 4> hat{};
};
/// DBSIM 0x4b828 and its original device specializations, at seven calibration bits.
joystick_sample sample_joystick(joystick_profile const &, joystick_calibration const &, std::array<int32_t, 4> const &raw, uint8_t buttons);
std::array<joystick_profile, 2> joystick_profiles(uint8_t model, bool pedals);
void combine_joystick(joystick_sample &, joystick_sample const &);
} // namespace earthsiege
