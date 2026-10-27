#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include "render/palette_shading.h"
#include "resources/binary_reader.h"
#include "resources/resource_store.h"

auto main(int const argc, char **argv)->int try {
  if(argc != 3) return EXIT_FAILURE;
  earthsiege::resource_store const store{argv[1]};
  auto const data{earthsiege::read_file(argv[2])};
  earthsiege::binary_reader file{data};
  auto const magic{file.take(8)};
  if(std::string(magic.begin(), magic.end()) != "ESSHADE1") file.fail("fixture version");
  auto const count{file.integer<uint16_t>()};
  unsigned int checked{0};
  for(unsigned int row{0}; row != count; ++row) {
    auto const name{file.cstring()};
    auto const separator{name.find('/')};
    auto const source{earthsiege::read_palette(store.read(name.substr(0, separator), name.substr(separator + 1)))};
    auto const maps{earthsiege::make_texture_shades(source)};
    for(unsigned int level{0}; level != maps.size(); ++level) {
      for(unsigned int index{0}; index != 256; ++index) {
        auto const expected{file.integer<uint8_t>()};
        if(maps[level][index] != expected) throw std::runtime_error{"texture shade differs: " + name + " level " + std::to_string(level) + " colour " + std::to_string(index)};
      }
    }
    auto const ramps{file.integer<uint16_t>()};
    for(unsigned int ramp{0}; ramp != ramps; ++ramp) {
      auto const index{file.integer<uint16_t>()};
      for(unsigned int intensity{0}; intensity != 256; ++intensity) {
        auto const expected{file.integer<uint8_t>()};
        if(earthsiege::shade_colour(source.shade_ramps.at(index), static_cast<uint8_t>(intensity)) != expected) file.fail("shade ramp differs from original renderer");
        ++checked;
      }
    }
  }
  file.require_end();
  if(count != 18 || checked != 32256) file.fail("incomplete palette shading fixture");
  std::cout << count * 8 << " native texture maps and " << checked << " verified ramp lookups agree" << std::endl;
  return EXIT_SUCCESS;
} catch(std::exception const &error) {
  std::cerr << error.what() << std::endl;
  return EXIT_FAILURE;
}
