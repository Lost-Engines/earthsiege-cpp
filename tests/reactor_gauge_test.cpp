#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include "render/instrument_bars.h"

int main(int argc, char **argv) try {
  using namespace earthsiege;
  if(argc != 3) return EXIT_FAILURE;
  resource_store resources{argv[1]};
  auto const bytes{read_file(argv[2])}; binary_reader file{bytes};
  auto const magic{file.take(8)};
  if(std::string(magic.begin(), magic.end()) != "ESREACT1") file.fail("fixture version");
  auto const count{file.integer<uint32_t>()};
  unsigned int failed{};
  for(unsigned int i{}; i < count; ++i) {
    auto const name{file.cstring()}; auto const value{file.integer<int32_t>()}; auto const expected{file.integer<uint32_t>()};
    auto const layout{read_cockpit_layout(resources.read("simvol0", "gau/" + name + ".GAU"))};
    reactor_gauge gauge{resources, layout};
    framebuffer frame; frame.clear(123); gauge.draw(frame, value);
    if(crc32(frame.pixels()) != expected) { ++failed; std::cerr << name << " value " << value << " differs: " << crc32(frame.pixels()) << " expected " << expected << '\n'; }
  }
  file.require_end();
  if(failed) throw std::runtime_error{std::to_string(failed) + " reactor frames differ"};
  std::cout << count << " original reactor instrument frames match, including shaped cutouts\n";
  return EXIT_SUCCESS;
} catch(std::exception const &e) { std::cerr << e.what() << '\n'; return EXIT_FAILURE; }
