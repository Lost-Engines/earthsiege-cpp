#include <array>
#include <cstdlib>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>
#include "resources/binary_reader.h"
#include "resources/model.h"
#include "resources/resource_store.h"
#include "simulation/animation_channel.h"

namespace {

void require(bool const condition, unsigned int const row, int const step, std::string const &field) {
  if(!condition) throw std::runtime_error{"animation channel differs at trace " + std::to_string(row) + ", step " + std::to_string(step) + ": " + field};
}

void check_transform(earthsiege::binary_reader &file, earthsiege::model_transform const &transform, unsigned int const row, int const step, std::string const &name) {
  for(unsigned int const index : {0U, 1U, 3U, 4U, 2U, 5U, 6U, 7U, 8U}) require(transform.basis.data[index] == file.integer<int16_t>(), row, step, name + " basis");
  require(transform.type == file.integer<uint8_t>(), row, step, name + " type");
  file.integer<uint8_t>();                                                      // unwritten original matrix padding
  for(unsigned int axis{0}; axis != 3; ++axis) require(transform.translation[axis] == file.integer<int32_t>(), row, step, name + " translation");
}

void check_state(earthsiege::binary_reader &file, earthsiege::animation_channel const &channel, unsigned int const row, int const step) {
  auto const &s{channel.state()};
  std::array<int16_t, 15> const fields{
    s.sequence, s.frame, s.next_sequence, s.next_frame, s.exit_frame,
    s.destination_sequence, s.destination_frame, s.transition_duration, s.transition_pose,
    s.requested_sequence, -1, static_cast<int16_t>(s.stop_on_arrival), s.elapsed,
    s.duration, static_cast<int16_t>(s.has_motion),
  };
  for(size_t index{0}; index != fields.size(); ++index) {
    auto const expected{file.integer<int16_t>()};
    require(fields[index] == expected, row, step, "field " + std::to_string(index) + ": got " + std::to_string(fields[index]) + ", expected " + std::to_string(expected));
  }
  for(unsigned int axis{0}; axis != 3; ++axis) require(s.motion.translation[axis] == file.integer<int16_t>(), row, step, "movement translation");
  for(unsigned int axis{0}; axis != 3; ++axis) require(s.motion.rotation[axis] == file.integer<uint16_t>(), row, step, "movement rotation");
  check_transform(file, s.accumulated, row, step, "accumulated movement");
  require(s.rate == file.integer<int16_t>(), row, step, "playback rate");
  require(static_cast<uint16_t>(s.flags) == file.integer<uint16_t>(), row, step, "flags");
  if(file.integer<uint8_t>() != 0) check_transform(file, channel.root_motion(), row, step, "interpolated movement");
}

} // anonymous namespace

auto main(int const argc, char **argv)->int try {
  if(argc != 3) return EXIT_FAILURE;
  earthsiege::resource_store resources{argv[1]};
  auto const bytes{earthsiege::read_file(argv[2])};
  earthsiege::binary_reader file{bytes};
  auto const magic{file.take(8)};
  require(std::string(magic.begin(), magic.end()) == "ESCHAN01", 0, 0, "fixture version");
  auto const count{file.integer<uint32_t>()};
  auto const step_count{file.integer<uint16_t>()};
  std::vector<int16_t> steps;
  for(unsigned int step{0}; step != step_count; ++step) steps.push_back(file.integer<int16_t>());
  std::map<std::string, earthsiege::model_resource> models;
  for(unsigned int row{0}; row != count; ++row) {
    auto const name{file.cstring()};
    auto const split{name.find('/')};
    if(!models.contains(name)) models.emplace(name, earthsiege::read_model(resources.read(name.substr(0, split), name.substr(split + 1))));
    auto const root{file.integer<uint16_t>()};
    auto const sequence{file.integer<int16_t>()};
    auto const frame{file.integer<int16_t>()};
    auto const elapsed{file.integer<int16_t>()};
    auto const rate{file.integer<int16_t>()};
    auto const target{file.integer<int16_t>()};
    bool const stop{file.integer<int16_t>() != 0};
    auto const &shape{std::get<earthsiege::shape_part>(models.at(name).roots.at(root).content)};
    earthsiege::animation_channel channel{*shape.animation};
    channel.reset(sequence, frame, elapsed);
    channel.set_rate(rate);
    check_state(file, channel, row, -1);
    channel.request_sequence(target, stop);
    check_state(file, channel, row, 0);
    for(unsigned int step{0}; step != steps.size(); ++step) {
      channel.advance(steps[step]);
      check_state(file, channel, row, static_cast<int>(step + 1));
    }
  }
  file.require_end();
  std::cout << count << " original animation traces, " << count * (steps.size() + 1) << " transition states and 5120 root-motion samples agree" << std::endl;
  return EXIT_SUCCESS;
} catch(std::exception const &error) {
  std::cerr << error.what() << std::endl;
  return EXIT_FAILURE;
}
