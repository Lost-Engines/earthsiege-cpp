#include <cstdlib>
#include <iostream>
#include <fstream>
#include <memory>
#include <stdexcept>
#include "render/navigation_instruments.h"

int main(int argc, char **argv) try {
  using namespace earthsiege;
  if(argc != 3) return EXIT_FAILURE;
  resource_store resources{argv[1]};
  auto const source{read_file(argv[2])}; binary_reader file{source}; auto const magic{file.take(8)};
  if(std::string(magic.begin(), magic.end()) != "ESHUDN03") file.fail("fixture version");
  auto const count{file.integer<uint32_t>()};
  std::string chassis; std::unique_ptr<navigation_instruments> hud;
  for(unsigned int i{}; i < count; ++i) {
    auto const name{file.cstring()};
    if(name != chassis) {
      chassis = name;
      hud = std::make_unique<navigation_instruments>(resources, read_cockpit_layout(resources.read("simvol0", "gau/" + name + ".GAU")));
    }
    auto const heading{file.integer<int16_t>()}, torso{file.integer<int16_t>()}, pitch{file.integer<int16_t>()};
    bool const disabled{file.integer<uint8_t>() != 0};
    bool const tracking{file.integer<uint8_t>() != 0};
    std::array<int16_t, 2> const indicators{file.integer<int16_t>(), file.integer<int16_t>()};
    auto const expected{file.integer<uint32_t>()};
    hud->update(heading,torso,pitch,disabled);
    if(hud->indicator_angles() != indicators) file.fail(name + " indicator state differs at " + std::to_string(i));
    framebuffer frame; frame.clear(123); hud->draw(frame,0,0,tracking);
    if(crc32(frame.pixels()) != expected) {
      std::ofstream output{"/tmp/es-navigation-difference.raw", std::ios::binary};
      output.write(reinterpret_cast<char const *>(frame.pixels().data()),frame.pixels().size());
      file.fail(name + " navigation paint differs at " + std::to_string(i) + ": " + std::to_string(crc32(frame.pixels())) + " expected " + std::to_string(expected));
    }
  }
  file.require_end(); std::cout << count << " original navigation instrument frames match\n";
  return EXIT_SUCCESS;
} catch(std::exception const &e) { std::cerr << e.what() << '\n'; return EXIT_FAILURE; }
