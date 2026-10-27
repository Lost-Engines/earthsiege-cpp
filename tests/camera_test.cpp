#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include "resources/binary_reader.h"
#include "simulation/camera.h"

namespace {

template<typename T> vec3<T> vector(earthsiege::binary_reader &file) {
  return vec3<T>{file.integer<T>(), file.integer<T>(), file.integer<T>()};
}

earthsiege::model_transform matrix(earthsiege::binary_reader &file) {
  earthsiege::model_transform result;
  for(unsigned int const field : {0U, 1U, 3U, 4U, 2U, 5U, 6U, 7U, 8U}) result.basis.data[field] = file.integer<int16_t>();
  result.type = file.integer<uint8_t>();
  file.integer<uint8_t>();
  result.translation = vector<int32_t>(file);
  return result;
}

earthsiege::camera_state state(earthsiege::binary_reader &file) {
  return {
    .position{vector<int32_t>(file)}, .angles{vector<uint16_t>(file)},
    .orbit_distance{file.integer<int16_t>()}, .movement{file.integer<int16_t>()},
    .orbit_angles{vector<uint16_t>(file)}, .angular_rates{vector<int16_t>(file)},
    .mode{static_cast<earthsiege::camera_mode>(file.integer<int16_t>())},
    .focus_offset{vector<int16_t>(file)}, .attachment_offset{vector<int16_t>(file)}, .look_offset{vector<int16_t>(file)},
  };
}

} // anonymous namespace

int main(int const argc, char **argv) try {
  if(argc != 2) return EXIT_FAILURE;
  auto const bytes{earthsiege::read_file(argv[1])};
  earthsiege::binary_reader file{bytes};
  auto const magic{file.take(8)};
  if(std::string(magic.begin(), magic.end()) != "ESCAM001") file.fail("fixture version");
  auto const count{file.integer<uint32_t>()};
  for(unsigned int index{0}; index != count; ++index) {
    earthsiege::camera_target target{.angles{vector<uint16_t>(file)}, .placement{matrix(file)}, .attachment{std::nullopt}};
    if(file.integer<uint8_t>()) target.attachment = matrix(file);
    auto actual{state(file)};
    auto const expected{state(file)};
    earthsiege::update_camera(actual, target);
    auto const require{[&](bool const pass, char const *field){if(!pass) throw std::runtime_error{"camera case " + std::to_string(index) + ": " + field + " differs"};}};
    require(actual.position == expected.position, "position");
    require(actual.angles == expected.angles, "angles");
    require(actual.orbit_distance == expected.orbit_distance, "distance");
    require(actual.movement == expected.movement, "movement");
    require(actual.orbit_angles == expected.orbit_angles, "orbit");
    require(actual.angular_rates == expected.angular_rates, "rates");
    require(actual.mode == expected.mode && actual.focus_offset == expected.focus_offset && actual.attachment_offset == expected.attachment_offset && actual.look_offset == expected.look_offset, "attachment");
  }
  auto const controls{file.integer<uint32_t>()};
  for(unsigned int index{0}; index != controls; ++index) {
    auto actual{state(file)};
    bool const zoom{file.integer<uint8_t>() != 0};
    auto const tilt{file.integer<int16_t>()};
    auto const turn{file.integer<int16_t>()};
    auto const expected{state(file)};
    earthsiege::steer_camera(actual, zoom, tilt, turn);
    if(actual.movement != expected.movement || actual.angular_rates != expected.angular_rates) throw std::runtime_error{"camera control " + std::to_string(index) + " differs"};
  }
  file.require_end();
  if(count != 960 || controls != 1200) file.fail("camera coverage differs");
  std::cout << count << " original camera placements and " << controls << " control updates agree" << std::endl;
  return EXIT_SUCCESS;
} catch(std::exception const &error) {
  std::cerr << error.what() << std::endl;
  return EXIT_FAILURE;
}
