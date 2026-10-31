#include <cstdlib>
#include <iostream>
#include <fstream>
#include <memory>
#include <stdexcept>
#include "render/clock_instrument.h"

int main(int argc, char **argv) try {
  using namespace earthsiege;
  if(argc != 3) return EXIT_FAILURE;
  resource_store resources{argv[1]};
  auto const source{read_file(argv[2])}; binary_reader file{source}; auto const magic{file.take(8)};
  if(std::string(magic.begin(), magic.end()) != "ESCLOCK1") file.fail("fixture version");
  auto const count{file.integer<uint32_t>()};
  std::string chassis; std::unique_ptr<clock_instrument> clock;
  for(unsigned int i{}; i < count; ++i) {
    auto const name{file.cstring()};
    if(chassis != name) { chassis = name; clock = std::make_unique<clock_instrument>(resources, read_cockpit_layout(resources.read("simvol0", "gau/" + name + ".GAU")), 0, false); }
    auto const now{file.integer<uint32_t>()}; bool const full{file.integer<uint8_t>() != 0};
    auto const label{file.take(5)}; bool const colon{file.integer<uint8_t>() != 0};
    auto const deadline{file.integer<uint32_t>()}, expected{file.integer<uint32_t>()};
    if(full) clock->refresh(); else clock->update(now);
    if(clock->text() != std::string(label.begin(), label.end()) || clock->colon_visible() != colon || clock->next_update() != deadline)
      file.fail(name + " clock state differs at " + std::to_string(i));
    framebuffer frame; frame.clear(123); clock->draw(frame);
    if(crc32(frame.pixels()) != expected) {
      std::ofstream output{"/tmp/es-clock-difference.raw", std::ios::binary}; output.write(reinterpret_cast<char const *>(frame.pixels().data()), frame.pixels().size());
      file.fail(name + " clock paint differs at " + std::to_string(i));
    }
  }
  file.require_end(); std::cout << count << " original clock frames match\n"; return EXIT_SUCCESS;
} catch(std::exception const &e) { std::cerr << e.what() << '\n'; return EXIT_FAILURE; }
