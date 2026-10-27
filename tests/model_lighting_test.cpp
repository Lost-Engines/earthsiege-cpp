#include <array>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <vector>
#include "render/model_lighting.h"
#include "resources/binary_reader.h"

namespace {

template<typename T>
vec3<T> read_vector(earthsiege::binary_reader &file) {
  return {file.integer<T>(), file.integer<T>(), file.integer<T>()};
}

earthsiege::light read_light(earthsiege::binary_reader &file) {
  return {
    .kind{static_cast<earthsiege::light_kind>(file.integer<uint8_t>())},
    .intensity{file.integer<int32_t>()},
    .position{read_vector<int32_t>(file)},
    .direction{read_vector<int16_t>(file)},
    .attenuation{file.integer<int32_t>()},
    .scale{file.integer<int32_t>()},
  };
}

void require(bool const condition, unsigned int const row, char const *field) {
  if(!condition) throw std::runtime_error{"lighting differs at reference " + std::to_string(row) + ": " + field};
}

void check_light(earthsiege::light const &result, earthsiege::light const &expected, unsigned int const row) {
  require(result.kind == expected.kind && result.intensity == expected.intensity, row, "kind/intensity");
  require(result.position == expected.position && result.direction == expected.direction, row, "position/direction");
  require(result.attenuation == expected.attenuation && result.scale == expected.scale, row, "attenuation");
}

} // anonymous namespace

auto main(int const argc, char **argv)->int try {
  if(argc != 2) return EXIT_FAILURE;
  auto const bytes{earthsiege::read_file(argv[1])};
  earthsiege::binary_reader file{bytes};
  auto const magic{file.take(8)};
  require(std::string(magic.begin(), magic.end()) == "ESLIGHT1", 0, "fixture version");
  check_light(earthsiege::default_world_light(), read_light(file), 0);
  auto const sets{file.integer<uint16_t>()};
  std::vector<std::vector<earthsiege::light>> lights;
  for(unsigned int set{0}; set != sets; ++set) {
    auto const count{file.integer<uint16_t>()};
    auto &group{lights.emplace_back()};
    for(unsigned int index{0}; index != count; ++index) group.push_back(read_light(file));
  }
  auto const cases{file.integer<uint32_t>()};
  for(unsigned int row{0}; row != cases; ++row) {
    auto const set{file.integer<uint16_t>()};
    auto const point{read_vector<int32_t>(file)};
    auto const normal{read_vector<int16_t>(file)};
    for(auto const surface : {earthsiege::lighting_surface::model, earthsiege::lighting_surface::terrain, earthsiege::lighting_surface::unoriented}) {
      auto const expected{file.integer<uint8_t>()};
      require(earthsiege::light_intensity(lights.at(set), point, normal, surface) == expected, row, "light response");
    }
  }
  auto const transforms{file.integer<uint32_t>()};
  for(unsigned int row{0}; row != transforms; ++row) {
    earthsiege::model_transform inverse;
    for(unsigned int const index : {0U, 1U, 3U, 4U, 2U, 5U, 6U, 7U, 8U}) inverse.basis.data[index] = file.integer<int16_t>();
    inverse.type = file.integer<uint8_t>();
    inverse.translation = read_vector<int32_t>(file);
    auto const source{read_light(file)};
    auto const expected{read_light(file)};
    check_light(earthsiege::transform_light(source, inverse), expected, row);
  }
  auto const operations{file.integer<uint16_t>()};
  earthsiege::light_list active;
  std::array<earthsiege::light, 12> sources;
  for(unsigned int row{0}; row != operations; ++row) {
    auto const index{file.integer<uint16_t>()};
    bool const accepted{file.integer<uint8_t>() != 0};
    auto const count{file.integer<uint8_t>()};
    require(active.add(sources.at(index)) == accepted && active.lights().size() == count, row, "source registration");
  }
  active.remove(sources[3]);
  require(active.lights().size() == 9 && active.lights()[3] == &sources[9], operations, "swap-with-last removal");
  file.require_end();
  std::cout << cases * 3 << " original intensities, " << transforms << " light transforms and native list rules agree" << std::endl;
  return EXIT_SUCCESS;
} catch(std::exception const &error) {
  std::cerr << error.what() << std::endl;
  return EXIT_FAILURE;
}
