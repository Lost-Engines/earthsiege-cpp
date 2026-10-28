#include <cstdlib>
#include <fstream>
#include <iostream>
#include "render/navigation_display.h"
#include "resources/resource_store.h"

int main(int argc, char **argv) try {
  using namespace earthsiege;
  if(argc != 3) return EXIT_FAILURE;
  resource_store resources{argv[1]};
  auto const source{read_file(argv[2])}; binary_reader file{source};
  auto const magic{file.take(8)};
  if(std::string(magic.begin(), magic.end()) != "ESNAV001") file.fail("fixture version");
  auto const count{file.integer<uint16_t>()};
  unsigned int total{}, failures{};
  for(unsigned int i{}; i < count; ++i) {
    std::array<int16_t, 10> header;
    for(auto &value : header) value = file.integer<int16_t>();
    mission_header const mission{.terrain{static_cast<terrain_kind>(header[0])}, .zone{header[1]}, .world_variant{header[8]}, .palette_variant{header[9]}};
    scenery_world world{resources, mission}; framebuffer frame;
    navigation_display display{frame, world};
    std::array const lights{default_world_light()};
    auto const cases{file.integer<uint16_t>()};
    for(unsigned int j{}; j < cases; ++j) {
      rectangle const bounds{file.integer<int32_t>(), file.integer<int32_t>(), file.integer<int32_t>(), file.integer<int32_t>()};
      vec3<int32_t> const position{file.integer<int32_t>(), file.integer<int32_t>(), file.integer<int32_t>()};
      vec3<uint16_t> const angles{file.integer<uint16_t>(), file.integer<uint16_t>(), file.integer<uint16_t>()};
      auto const expected{file.integer<uint32_t>()};
      frame.clear(123); display.draw(bounds, position, angles, lights);
      if(crc32(frame.pixels()) != expected) {
        if(!failures) {
          std::ofstream output{"/tmp/es-navigation-difference.raw", std::ios::binary};
          output.write(reinterpret_cast<char const *>(frame.pixels().data()), frame.pixels().size());
        }
        ++failures; std::cerr << "world " << i << " map " << j << " differs\n";
      }
      ++total;
    }
  }
  file.require_end();
  if(failures) file.fail(std::to_string(failures) + " navigation display frames differ");
  std::cout << total << " original navigation frames match\n";
  return EXIT_SUCCESS;
} catch(std::exception const &e) { std::cerr << e.what() << '\n'; return EXIT_FAILURE; }
