#pragma once
#include <array>
#include <stdexcept>
#include <string>
#include <vector>
#include "resources/binary_reader.h"
#include "simulation/herc_body.h"

namespace herc_reference {

struct reference_state {
  vec3<int32_t> position;
  vec3<uint16_t> angles;
  std::array<int16_t, 8> values;
  std::vector<earthsiege::animation_pose> poses;
  std::vector<earthsiege::model_transform> matrices;
};

inline reference_state read_state(earthsiege::binary_reader &file) {
  reference_state result;
  for(unsigned int axis{0}; axis != 3; ++axis) result.position[axis] = file.integer<int32_t>();
  for(unsigned int axis{0}; axis != 3; ++axis) result.angles[axis] = file.integer<uint16_t>();
  for(auto &value : result.values) value = file.integer<int16_t>();
  auto const nodes{file.integer<uint16_t>()};
  result.poses.resize(nodes);
  result.matrices.resize(nodes);
  for(auto &pose : result.poses) {
    for(unsigned int axis{0}; axis != 3; ++axis) pose.rotation[axis] = file.integer<uint16_t>();
    for(unsigned int axis{0}; axis != 3; ++axis) pose.translation[axis] = file.integer<int16_t>();
  }
  for(auto &matrix : result.matrices) {
    for(unsigned int const field : {0U, 1U, 3U, 4U, 2U, 5U, 6U, 7U, 8U}) matrix.basis.data[field] = file.integer<int16_t>();
    matrix.type = file.integer<uint8_t>();
    file.integer<uint8_t>();
    for(unsigned int axis{0}; axis != 3; ++axis) matrix.translation[axis] = file.integer<int32_t>();
  }
  return result;
}

inline void check(earthsiege::herc_body const &body, reference_state const &expected, std::string const &label) {
  auto const require{[&](bool const pass, std::string const &field){
    if(!pass) throw std::runtime_error{label + ": " + field + " differs"};
  }};
  auto const &movement{body.movement()};
  require(movement.position == expected.position, "position");
  require(movement.angles == expected.angles, "angles");
  std::array<int16_t, 8> const values{movement.steering_rate, movement.gait_rate, movement.throttle, movement.playback_rate, body.torso().rate, body.elevation().rate, body.torso().angle, body.elevation().angle};
  for(size_t i{0}; i != values.size(); ++i) require(values[i] == expected.values[i], "control field " + std::to_string(i));
  require(body.model().poses().size() == expected.poses.size(), "pose count");
  for(size_t i{0}; i != expected.poses.size(); ++i) {
    auto const &a{body.model().poses()[i]};
    auto const &b{expected.poses[i]};
    require(a.rotation == b.rotation && a.translation == b.translation, "pose " + std::to_string(i));
    auto const &c{body.model().transforms()[i]};
    auto const &d{expected.matrices[i]};
    require(c.basis == d.basis && c.translation == d.translation && c.type == d.type, "transform " + std::to_string(i));
  }
}

} // namespace herc_reference
