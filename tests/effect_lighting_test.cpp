#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <vector>
#include "render/effect_lighting.h"
#include "resources/binary_reader.h"

namespace {

template<typename T>
vec3<T> vector(earthsiege::binary_reader &file) { return {file.integer<T>(), file.integer<T>(), file.integer<T>()}; }

earthsiege::light light(earthsiege::binary_reader &file) {
  return {.kind{static_cast<earthsiege::light_kind>(file.integer<uint8_t>())}, .intensity{file.integer<int32_t>()}, .position{vector<int32_t>(file)}, .direction{vector<int16_t>(file)}, .attenuation{file.integer<int32_t>()}, .scale{file.integer<int32_t>()}};
}

void require(bool const value, size_t const row, char const *field) {
  if(!value) throw std::runtime_error{"effect lighting differs at " + std::to_string(row) + ": " + field};
}

void compare(earthsiege::light const &value, earthsiege::light const &expected, size_t const row) {
  require(value.kind == expected.kind && value.intensity == expected.intensity, row, "kind/intensity");
  require(value.position == expected.position && value.direction == expected.direction, row, "position/direction");
  require(value.attenuation == expected.attenuation && value.scale == expected.scale, row, "attenuation");
}

} // anonymous namespace

int main(int const argc, char **argv) try {
  if(argc != 2) return EXIT_FAILURE;
  auto const bytes{earthsiege::read_file(argv[1])};
  earthsiege::binary_reader file{bytes};
  auto const magic{file.take(8)};
  require(std::string(magic.begin(), magic.end()) == "ESFXLT01", 0, "version");
  auto const isolated{file.integer<uint32_t>()};
  for(size_t row{0}; row != isolated; ++row) {
    auto const source{vector<int32_t>(file)}, object{vector<int32_t>(file)};
    auto const radius{file.integer<int32_t>()};
    auto const intensity{file.integer<int16_t>()};
    bool const present{file.integer<uint8_t>() != 0};
    earthsiege::effect_lighting manager{{}};
    manager.intensity(manager.add(source).value(), intensity);
    auto const selected{manager.select(object, radius)};
    require(selected.size() == static_cast<size_t>(present), row, "isolated count");
    if(present) compare(selected.front(), light(file), row);
  }
  size_t operations{0};
  auto const traces{file.integer<uint32_t>()};
  for(size_t trace{0}; trace != traces; ++trace) {
    auto const scale{file.integer<int32_t>()}, offset{file.integer<int32_t>()};
    auto const fixed_count{file.integer<uint8_t>()};
    std::vector<earthsiege::light> persistent;
    for(size_t i{0}; i != fixed_count; ++i) persistent.push_back(light(file));
    earthsiege::effect_lighting manager{persistent, scale, offset};
    auto const count{file.integer<uint32_t>()};
    for(size_t row{0}; row != count; ++row, ++operations) {
      switch(file.integer<uint8_t>()) {
      case 0: {
        auto const position{vector<int32_t>(file)};
        auto const slot{file.integer<uint8_t>()};
        require(manager.add(position) == slot, operations, "allocation slot");
        break;
      }
      case 1: manager.remove(file.integer<uint8_t>()); break;
      case 2: {
        auto const slot{file.integer<uint8_t>()};
        manager.intensity(slot, file.integer<int16_t>());
        break;
      }
      case 3: {
        auto const slot{file.integer<uint8_t>()};
        auto const strength{file.integer<int32_t>()}, attenuation{file.integer<int32_t>()};
        manager.attenuation(slot, strength, attenuation);
        break;
      }
      case 4: {
        auto const position{vector<int32_t>(file)};
        auto const radius{file.integer<int32_t>()};
        auto const expected{file.integer<uint8_t>()};
        auto const selected{manager.select(position, radius)};
        require(selected.size() == expected, operations, "active count");
        for(auto const &value : selected) compare(value, light(file), operations);
        break;
      }
      default: throw std::runtime_error{"invalid light fixture operation"};
      }
    }
  }
  file.require_end();
  std::cout << isolated << " isolated light selections and " << operations << " live light operations agree\n";
  return EXIT_SUCCESS;
} catch(std::exception const &error) {
  std::cerr << error.what() << '\n';
  return EXIT_FAILURE;
}
