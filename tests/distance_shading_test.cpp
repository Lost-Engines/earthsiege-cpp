#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include "render/distance_shading.h"
#include "resources/resource_store.h"

int main(int const argc, char **argv) try {
  if(argc != 3) return EXIT_FAILURE;
  earthsiege::resource_store resources{argv[1]};
  auto const bytes{earthsiege::read_file(argv[2])};
  earthsiege::binary_reader file{bytes};
  auto const magic{file.take(8)};
  if(std::string(magic.begin(), magic.end()) != "ESDIST01") file.fail("fixture version");
  auto const worlds{file.integer<uint16_t>()};
  unsigned int checked{0};
  for(unsigned int row{0}; row != worlds; ++row) {
    auto const world{file.integer<uint16_t>()};
    auto const environment{earthsiege::read_world_environment(resources.read("simvol0", "wld/WORLD" + std::to_string(world) + ".WLD"))};
    auto colours{earthsiege::read_palette(resources.read("simvol0", "dpl/WORLD" + std::to_string(world - world % 2) + ".DPL"))};
    earthsiege::distance_shading const shading{environment, earthsiege::read_distance_shade_bindings(resources.read("simvol0", "dat/SHADES.DAT"))};
    auto const count{file.integer<uint32_t>()};
    for(unsigned int index{0}; index != count; ++index) {
      auto const kind{file.integer<int16_t>()};
      auto const distance{file.integer<int32_t>()};
      auto const radius{file.integer<int16_t>()};
      bool const split{file.integer<uint8_t>() != 0};
      auto const level{file.integer<uint16_t>()};
      bool const culled{file.integer<uint8_t>() != 0};
      auto const require{[&](bool const success, std::string const &field) {
        if(!success) throw std::runtime_error{"world " + std::to_string(world) + " distance case " + std::to_string(index) + ' ' + field + " differs"};
      }};
      require(shading.band(distance, kind, radius) == level, "band");
      require(shading.culled(distance, kind, radius) == culled, "culling");
      shading.apply(colours, distance, kind, radius, split);
      auto const slots{file.integer<uint8_t>()};
      for(unsigned int slot{0}; slot != slots; ++slot) {
        auto const target{file.integer<uint8_t>()};
        auto const size{file.integer<uint16_t>()};
        auto const &ramp{colours.shade_ramps.at(target)};
        require(ramp.size() == size, "palette ramp size");
        for(auto const value : ramp) require(value == file.integer<uint16_t>(), "palette entry");
      }
      ++checked;
    }
  }
  file.require_end();
  if(worlds != 24 || checked != 4800) file.fail("incomplete distance shading fixtures");
  std::cout << checked << " original distance/culling/palette cases agree across all " << worlds << " environments" << std::endl;
  return EXIT_SUCCESS;
} catch(std::exception const &error) {
  std::cerr << error.what() << std::endl;
  return EXIT_FAILURE;
}
