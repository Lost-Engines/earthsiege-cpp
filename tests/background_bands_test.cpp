#include <cstdlib>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include "render/background_bands.h"
#include "resources/artwork.h"
#include "resources/resource_store.h"

int main(int const argc, char **argv) try {
  if(argc != 3) return EXIT_FAILURE;
  earthsiege::resource_store resources{argv[1]};
  auto const bytes{earthsiege::read_file(argv[2])};
  earthsiege::binary_reader file{bytes};
  auto const magic{file.take(8)};
  if(std::string(magic.begin(), magic.end()) != "ESBANDS2") file.fail("fixture version");
  auto const worlds{file.integer<uint16_t>()};
  unsigned int total{0}, original_faults{0}, frames{0};
  for(unsigned int index{0}; index != worlds; ++index) {
    auto const world{file.integer<uint16_t>()};
    auto const environment{earthsiege::read_world_environment(resources.read("simvol0", "wld/WORLD" + std::to_string(world) + ".WLD"))};
    auto const count{file.integer<uint16_t>()};
    for(unsigned int row{0}; row != count; ++row) {
      earthsiege::background_view view;
      view.pitch = file.integer<uint16_t>();
      view.roll = file.integer<uint16_t>();
      view.projection_shift = file.integer<uint16_t>();
      view.centre = vec2<int32_t>{file.integer<int32_t>(), file.integer<int32_t>()};
      view.offset = vec2<int32_t>{file.integer<int32_t>(), file.integer<int32_t>()};
      bool const fault{file.integer<uint8_t>() != 0};
      bool const has_pixels{file.integer<uint8_t>() != 0};
      auto const pixels_crc{file.integer<uint32_t>()};
      auto const label{"world " + std::to_string(world) + " frame " + std::to_string(row)};
      auto const require{[&](bool const pass, std::string const &field){
        if(!pass) throw std::runtime_error{label + ": " + field + " differs"};
      }};
      earthsiege::background_bands actual;
      try {
        actual = earthsiege::make_background_bands(environment, view);
        require(!fault, "expected original undefined horizon");
      } catch(std::domain_error const &) {
        require(fault, "unexpected undefined horizon");
        ++original_faults;
        continue;
      }
      for(auto const &point : actual.horizon) {
        auto const expected_x{file.integer<int32_t>()};
        require(point.x == expected_x, "horizon x actual " + std::to_string(point.x) + " expected " + std::to_string(expected_x));
        auto const expected_y{file.integer<int32_t>()};
        require(point.y == expected_y, "horizon y actual " + std::to_string(point.y) + " expected " + std::to_string(expected_y));
      }
      require(actual.primitives.size() == file.integer<uint16_t>(), "command count");
      size_t command{0};
      for(auto const &primitive : actual.primitives) {
        auto const detail{"command " + std::to_string(command++)};
        require(static_cast<uint8_t>(primitive.kind) == file.integer<uint8_t>(), detail + " kind");
        require(primitive.colour == file.integer<uint32_t>(), detail + " colour");
        require(primitive.points.size() == file.integer<uint16_t>(), detail + " points");
        for(auto const &point : primitive.points) {
          require(point.x == file.integer<int32_t>(), detail + " x");
          require(point.y == file.integer<int32_t>(), detail + " y");
        }
      }
      if(has_pixels) {
        earthsiege::framebuffer frame;
        earthsiege::draw_background_bands(frame, actual, view.bounds);
        if(earthsiege::crc32(frame.pixels()) != pixels_crc) {
          auto const colours{earthsiege::read_palette(resources.read("simvol0", "dpl/WORLD" + std::to_string(world - world % 2) + ".DPL"))};
          frame.save("background-mismatch.ppm", colours);
          std::ofstream raw{"background-mismatch.bin", std::ios::binary};
          raw.write(reinterpret_cast<char const *>(frame.pixels().data()), static_cast<std::streamsize>(frame.pixels().size()));
          require(false, "pixels");
        }
        ++frames;
      }
      ++total;
    }
  }
  file.require_end();
  std::cout << total << " original background command sequences and " << frames << " complete frames agree; " << original_faults << " original undefined horizon angles are detected" << std::endl;
  return EXIT_SUCCESS;
} catch(std::exception const &error) {
  std::cerr << error.what() << std::endl;
  return EXIT_FAILURE;
}
