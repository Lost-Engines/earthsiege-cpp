#include <cstdlib>
#include <iostream>
#include "render/palette_device.h"
using earthsiege::binary_reader;
using earthsiege::palette;
using earthsiege::palette_device;
using earthsiege::read_file;
int main(int argc, char **argv) try {
  if(argc != 2) return EXIT_FAILURE;
  auto raw = read_file(argv[1]);
  binary_reader f{raw};
  auto magic = f.take(8);
  if(std::string(magic.begin(), magic.end()) != "ESDAC001") f.fail("palette fixture");
  std::array<palette, 2> sources;
  for(auto &p : sources) {
    for(auto &c : p.dac_colours) {
      for(auto &v : c) v = f.integer<uint8_t>();
    }
  }
  palette_device display;
  auto count = f.integer<uint32_t>();
  for(unsigned i{0}; i < count; ++i) {
    auto source = f.integer<uint8_t>();
    auto changes = f.integer<uint8_t>();
    auto &p{sources.at(source)};
    for(unsigned j{0}; j < changes; ++j) {
      auto first = f.integer<uint16_t>();
      auto n = f.integer<uint16_t>();
      for(unsigned k{first}; k < first + n; ++k) {
        for(auto &v : p.dac_colours.at(k)) v = f.integer<uint8_t>();
      }
      display.changed(first, n);
    }
    display.present(p);
    if(display.colours().dac_colours != p.dac_colours) f.fail("displayed palette at step " + std::to_string(i));
    for(size_t entry{0}; entry < p.dac_colours.size(); ++entry) {
      if(display.colours().colours[entry] != earthsiege::display_colour(p.dac_colours[entry])) f.fail("displayed RGB");
    }
    for(auto const &c : display.dac_registers()) {
      for(auto v : c) {
        if(v != f.integer<uint8_t>()) f.fail("DAC component at step " + std::to_string(i));
      }
    }
    if(display.next_index() != f.integer<uint8_t>()) f.fail("DAC cursor");
  }
  f.require_end();
  std::cout << count << " original palette-manager/DAC states match\n";
  return EXIT_SUCCESS;
} catch(std::exception const &e) {
  std::cerr << e.what() << '\n';
  return EXIT_FAILURE;
}
