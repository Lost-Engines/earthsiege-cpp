#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <vector>
#include "render/shield_instruments.h"

int main(int argc, char **argv) try {
  using earthsiege::binary_reader;
  using earthsiege::crc32;
  using earthsiege::framebuffer;
  using earthsiege::palette;
  using earthsiege::read_cockpit_layout;
  using earthsiege::read_file;
  using earthsiege::resource_store;
  using earthsiege::shield_instruments;
  using earthsiege::update_shield_colours;
  if(argc != 3) return EXIT_FAILURE;
  resource_store resources{argv[1]};
  auto const bytes{read_file(argv[2])};
  binary_reader file{bytes};
  auto const magic{file.take(8)};
  if(std::string(magic.begin(), magic.end()) != "ESSHLD01") file.fail("fixture version");
  auto const count{file.integer<uint32_t>()};
  unsigned int failed{0};
  palette colours;
  auto const check_palette{[&]{
    for(size_t slot{80}; slot < 86; ++slot) {
      for(auto component : colours.dac_colours[slot]) {
        if(component != file.integer<uint8_t>()) file.fail("shield DAC value differs");
      }
    }
  }};
  for(unsigned int i{0}; i < count; ++i) {
    auto const name{file.cstring()};
    auto const front{file.integer<int32_t>()};
    auto const rear{file.integer<int32_t>()};
    auto const balance{file.integer<int32_t>()};
    std::array<unsigned int, 2> const faces{file.integer<uint8_t>(), file.integer<uint8_t>()};
    auto const expected{file.integer<uint32_t>()};
    auto const layout{read_cockpit_layout(resources.read("simvol0", "gau/" + name + ".GAU"))};
    shield_instruments instruments{resources, layout};
    framebuffer frame;
    frame.clear(123);
    instruments.draw(frame, balance, faces);
    if(crc32(frame.pixels()) != expected) {
      ++failed;
      if(failed < 12) std::cerr << name << " balance " << balance << " differs: " << crc32(frame.pixels()) << " expected " << expected << '\n';
    }
    update_shield_colours(colours, front, rear, resources.edition());
    check_palette();
  }
  auto const palette_count{file.integer<uint32_t>()};
  for(unsigned int i{0}; i < palette_count; ++i) {
    auto const front{file.integer<int32_t>()};
    auto const rear{file.integer<int32_t>()};
    update_shield_colours(colours, front, rear, resources.edition());
    check_palette();
  }
  file.require_end();
  std::vector<uint8_t> expansion_dac;
  for(int charge{0}; charge <= 1024; ++charge) {
    update_shield_colours(colours, charge, 1024 - charge, earthsiege::game_edition::expansion);
    for(size_t slot{80}; slot < 86; ++slot) {
      auto const &rgb{colours.dac_colours[slot]};
      expansion_dac.insert(expansion_dac.end(), rgb.begin(), rgb.end());
    }
  }
  // RGB6 bytes from all six slots, identical in the expansion CD, floppy and patch.
  if(crc32(expansion_dac) != 0xce6cffe5u) throw std::runtime_error{"expansion shield DAC values differ"};
  if(failed) throw std::runtime_error{std::to_string(failed) + " shield frames differ"};
  std::cout << count << " original shield frames, " << palette_count << " retail palettes and 1025 expansion palettes match\n";
  return EXIT_SUCCESS;
} catch(std::exception const &e) {
  std::cerr << e.what() << '\n';
  return EXIT_FAILURE;
}
