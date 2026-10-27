#include <array>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include "resources/resource_store.h"
#include "simulation/locomotion.h"
#include "simulation/model_math.h"

namespace {

void require(bool const success, std::string const &label, std::string const &field) {
  if(!success) throw std::runtime_error{label + ": " + field + " differs"};
}

earthsiege::locomotion_state read_state(earthsiege::binary_reader &file) {
  return earthsiege::locomotion_state{
    .position{file.integer<int32_t>(), file.integer<int32_t>(), file.integer<int32_t>()},
    .angles{file.integer<uint16_t>(), file.integer<uint16_t>(), file.integer<uint16_t>()},
    .throttle{file.integer<int16_t>()},
    .gait_rate{file.integer<int16_t>()},
    .steering_rate{file.integer<int16_t>()},
    .playback_rate{file.integer<int16_t>()},
  };
}

void check_channel(earthsiege::binary_reader &file, earthsiege::animation_channel const &channel, std::string const &label) {
  auto const &s{channel.state()};
  std::array<int16_t, 15> const fields{
    s.sequence, s.frame, s.next_sequence, s.next_frame, s.exit_frame,
    s.destination_sequence, s.destination_frame, s.transition_duration, s.transition_pose,
    s.requested_sequence, -1, static_cast<int16_t>(s.stop_on_arrival), s.elapsed,
    s.duration, static_cast<int16_t>(s.has_motion),
  };
  for(size_t index{0}; index != fields.size(); ++index) {
    auto const expected{file.integer<int16_t>()};
    require(fields[index] == expected, label, "channel " + std::to_string(index) + " got " + std::to_string(fields[index]) + " expected " + std::to_string(expected));
  }
  require(static_cast<uint16_t>(s.flags) == file.integer<uint16_t>(), label, "channel flags");
}

void check_state(earthsiege::locomotion_state const &actual, earthsiege::locomotion_state const &expected, std::string const &label) {
  require(actual.position == expected.position, label, "position");
  require(actual.angles == expected.angles, label, "angles");
  require(actual.throttle == expected.throttle, label, "throttle");
  require(actual.gait_rate == expected.gait_rate, label, "gait rate");
  require(actual.steering_rate == expected.steering_rate, label, "steering rate");
  require(actual.playback_rate == expected.playback_rate, label, "playback rate");
}

} // anonymous namespace

int main(int const argc, char **argv) try {
  if(argc != 4) return EXIT_FAILURE;
  earthsiege::resource_store resources{argv[1]};
  auto const angles{earthsiege::read_file(argv[3])};
  earthsiege::binary_reader angle_file{angles};
  auto const angle_magic{angle_file.take(8)};
  require(std::string(angle_magic.begin(), angle_magic.end()) == "ESANGLE1", "angles", "fixture version");
  auto const angle_count{angle_file.integer<uint32_t>()};
  for(unsigned int row{0}; row != angle_count; ++row) {
    auto const x{angle_file.integer<int32_t>()};
    auto const y{angle_file.integer<int32_t>()};
    auto const expected{angle_file.integer<uint16_t>()};
    require(earthsiege::direction_angle(x, y) == expected, "angle " + std::to_string(row), "direction");
  }
  angle_file.require_end();
  auto const bytes{earthsiege::read_file(argv[2])};
  earthsiege::binary_reader file{bytes};
  auto const magic{file.take(8)};
  require(std::string(magic.begin(), magic.end()) == "ESLOCO01", "locomotion", "fixture version");
  auto const models{file.integer<uint32_t>()};
  unsigned int updates{0};
  for(unsigned int row{0}; row != models; ++row) {
    auto const name{file.cstring()};
    auto const model{earthsiege::read_model(resources.read("simvol0", "dts/" + name + ".DTS"))};
    auto const profile{earthsiege::read_herc_profile(resources.read("simvol0", "dat/" + name + ".DAT"))};
    auto const &shape{std::get<earthsiege::shape_part>(model.roots.at(0).content)};
    auto const initial{read_state(file)};
    earthsiege::animation_channel initial_channel{*shape.animation};
    initial_channel.reset(profile.movement.forward_stop_sequence);
    check_channel(file, initial_channel, name + "/initial");
    auto const traces{file.integer<uint16_t>()};
    for(unsigned int trace{0}; trace != traces; ++trace) {
      auto const controls{file.cstring()};
      auto const ratio{file.integer<int16_t>()};
      auto const count{file.integer<uint16_t>()};
      auto state{initial};
      auto channel{initial_channel};
      for(unsigned int step{0}; step != count; ++step) {
        auto const axis{file.integer<int16_t>()};
        auto const steer{file.integer<int16_t>()};
        auto const expected{read_state(file)};
        auto const label{name + '/' + controls + '/' + std::to_string(ratio) + '/' + std::to_string(step)};
        earthsiege::update_player_movement(state, profile.movement, channel, axis, steer);
        earthsiege::advance_root_movement(state, channel, ratio);
        check_state(state, expected, label);
        check_channel(file, channel, label);
        ++updates;
      }
    }
  }
  file.require_end();
  require(models == 8 && updates == 11520 && angle_count == 4104, "coverage", "fixture counts");
  std::cout << updates << " original movement states agree across all eight playable HERCs; " << angle_count << " original direction angles agree" << std::endl;
  return EXIT_SUCCESS;
} catch(std::exception const &error) {
  std::cerr << error.what() << std::endl;
  return EXIT_FAILURE;
}
