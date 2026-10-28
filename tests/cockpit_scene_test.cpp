#include <cstdlib>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include "render/scenery_renderer.h"
#include "resources/cockpit.h"
#include "resources/resource_store.h"

int main(int const argc, char **argv) try {
  if(argc != 3) return EXIT_FAILURE;
  earthsiege::resource_store resources{argv[1]};
  auto const bytes{earthsiege::read_file(argv[2])};
  earthsiege::binary_reader file{bytes};
  auto const magic{file.take(8)};
  if(std::string(magic.begin(), magic.end()) != "ESCPWLD1") file.fail("fixture version");
  auto const worlds{file.integer<uint16_t>()};
  unsigned int failed{0}, total{0};
  auto const bindings{earthsiege::read_distance_shade_bindings(resources.read("simvol0", "dat/SHADES.DAT"))};
  for(unsigned int world_index{0}; world_index != worlds; ++world_index) {
    std::array<int16_t, 10> header;
    for(auto &value : header) value = file.integer<int16_t>();
    earthsiege::mission_header const mission{.terrain{static_cast<earthsiege::terrain_kind>(header[0])}, .zone{header[1]}, .world_variant{header[8]}, .palette_variant{header[9]}};
    earthsiege::scenery_world world{resources, mission};
    earthsiege::framebuffer frame;
    earthsiege::scenery_renderer renderer{frame, world, bindings};
    std::array const lights{earthsiege::default_world_light()};
    auto const count{file.integer<uint16_t>()};
    for(unsigned int row{0}; row != count; ++row) {
      auto const cockpit{earthsiege::read_cockpit(resources, file.cstring())};
      auto const &window{cockpit.windows.at(file.integer<uint8_t>())};
      vec3<int32_t> const position{file.integer<int32_t>(), file.integer<int32_t>(), file.integer<int32_t>()};
      vec3<uint16_t> const angles{file.integer<uint16_t>(), file.integer<uint16_t>(), file.integer<uint16_t>()};
      auto const expected{file.integer<uint32_t>()};
      std::vector<size_t> order(file.integer<uint16_t>());
      for(auto &value : order) value = file.integer<uint16_t>();
      frame.clear();
      earthsiege::scoped_framebuffer_clip clip{frame, &window.mask};
      auto const centre{window.projection_centre()};
      earthsiege::model_view view;
      view.bounds = window.bounds;
      view.projection.centre = {centre[0], centre[1]};
      auto const actual_order{renderer.draw(position, angles, lights, false, {}, nullptr, {}, view)};
      bool const order_matches{order == actual_order};
      bool const pixels_match{earthsiege::crc32(frame.pixels()) == expected};
      if(!order_matches || !pixels_match) {
        ++failed;
        std::cerr << "world " << world_index << " frame " << row << " order " << order_matches << " pixels " << pixels_match << " actual " << earthsiege::crc32(frame.pixels()) << " expected " << expected << '\n';
        if(failed == 1) {
          frame.save("cockpit-scene-mismatch.ppm", world.colours());
          std::ofstream raw{"cockpit-scene-mismatch.bin", std::ios::binary};
          raw.write(reinterpret_cast<char const *>(frame.pixels().data()), static_cast<std::streamsize>(frame.pixels().size()));
        }
      }
      ++total;
    }
  }
  file.require_end();
  if(failed) throw std::runtime_error{std::to_string(failed) + " complete scenery frames differ"};
  std::cout << total << " complete original scenery frames and painter orders agree" << std::endl;
  return EXIT_SUCCESS;
} catch(std::exception const &error) {
  std::cerr << error.what() << std::endl;
  return EXIT_FAILURE;
}
