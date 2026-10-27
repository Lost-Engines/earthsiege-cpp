#include <array>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include "resources/binary_reader.h"
#include "resources/resource_store.h"
#include "simulation/animation_rig.h"

namespace {

void require(bool const condition, std::string const &name, unsigned int const row, char const *field) {
  if(!condition) throw std::runtime_error{name + " layered pose " + std::to_string(row) + " differs: " + field};
}

} // namespace

int main(int const argc, char **argv) try {
  if(argc != 3) return EXIT_FAILURE;
  earthsiege::resource_store resources{argv[1]};
  auto const bytes{earthsiege::read_file(argv[2])};
  earthsiege::binary_reader file{bytes};
  auto const magic{file.take(8)};
  if(std::string(magic.begin(), magic.end()) != "ESRIGS01") file.fail("fixture version");
  auto const models{file.integer<uint16_t>()};
  unsigned int total{0};
  for(unsigned int index{0}; index != models; ++index) {
    auto const name{file.cstring()};
    auto model{earthsiege::read_model(resources.read("simvol0", "dts/" + name + ".DTS"))};
    auto &shape{std::get<earthsiege::shape_part>(model.roots[0].content)};
    auto const yaw_sequence{file.integer<int16_t>()};
    auto const pitch_sequence{file.integer<int16_t>()};
    auto const priorities{file.integer<uint16_t>()};
    require(priorities == shape.animation->sequences.size(), name, 0, "priority count");
    for(auto &sequence : shape.animation->sequences) sequence.priority = file.integer<int16_t>();
    earthsiege::animation_rig rig{shape};
    auto const body{rig.add_channel(0)};
    auto const yaw{rig.add_channel(yaw_sequence)};
    auto const pitch{rig.add_channel(pitch_sequence)};
    auto const cases{file.integer<uint16_t>()};
    for(unsigned int row{0}; row != cases; ++row) {
      auto const sequence{file.integer<int16_t>()};
      auto const frame{file.integer<int16_t>()};
      auto const elapsed{file.integer<int16_t>()};
      auto const yaw_angle{file.integer<int16_t>()};
      auto const pitch_angle{file.integer<int16_t>()};
      rig.instance().reset_pose();
      rig.channel(body).reset(sequence, frame, elapsed);
      rig.channel(yaw).set_phase(yaw_sequence, static_cast<int16_t>(static_cast<uint16_t>(yaw_angle) >> 2));
      rig.channel(pitch).set_phase(pitch_sequence, static_cast<int16_t>(static_cast<uint16_t>(pitch_angle) >> 2));
      for(size_t channel{0}; channel != 3; ++channel) {
        auto const &s{rig.channel(channel).state()};
        std::array<int16_t, 15> const fields{s.sequence, s.frame, s.next_sequence, s.next_frame, s.exit_frame, s.destination_sequence, s.destination_frame, s.transition_duration, s.transition_pose, s.requested_sequence, -1, static_cast<int16_t>(s.stop_on_arrival), s.elapsed, s.duration, static_cast<int16_t>(s.has_motion)};
        for(auto const value : fields) require(value == file.integer<int16_t>(), name, row, "channel state");
      }
      rig.update_pose();
      for(auto const channel : rig.order()) require(channel == file.integer<uint8_t>(), name, row, "priority order");
      auto const nodes{file.integer<uint16_t>()};
      require(nodes == rig.instance().poses().size(), name, row, "node count");
      for(auto const &pose : rig.instance().poses()) {
        for(unsigned int axis{0}; axis != 3; ++axis) require(pose.rotation[axis] == file.integer<uint16_t>(), name, row, "pose rotation");
        for(unsigned int axis{0}; axis != 3; ++axis) require(pose.translation[axis] == file.integer<int16_t>(), name, row, "pose translation");
      }
      for(auto const &matrix : rig.instance().transforms()) {
        for(unsigned int const field : {0U, 1U, 3U, 4U, 2U, 5U, 6U, 7U, 8U}) require(matrix.basis.data[field] == file.integer<int16_t>(), name, row, "matrix basis");
        require(matrix.type == file.integer<uint8_t>(), name, row, "matrix type");
        file.integer<uint8_t>();
        for(unsigned int axis{0}; axis != 3; ++axis) require(matrix.translation[axis] == file.integer<int32_t>(), name, row, "matrix translation");
      }
      ++total;
    }
  }
  file.require_end();
  if(total != 800) file.fail("incomplete layered pose fixtures");
  std::cout << total << " original walking/torso/elevation rigs match phases, priority and every node" << std::endl;
  return EXIT_SUCCESS;
} catch(std::exception const &error) {
  std::cerr << error.what() << std::endl;
  return EXIT_FAILURE;
}
