#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include "render/weapon_instruments.h"

int main(int argc, char **argv) try {
  using earthsiege::binary_reader;
  using earthsiege::crc32;
  using earthsiege::framebuffer;
  using earthsiege::read_cockpit_layout;
  using earthsiege::read_file;
  using earthsiege::rectangle;
  using earthsiege::resource_store;
  using earthsiege::screen_width;
  using earthsiege::weapon_instrument_kind;
  using earthsiege::weapon_instrument_state;
  using earthsiege::weapon_instruments;
  if(argc != 3) return EXIT_FAILURE;
  resource_store resources{argv[1]};
  auto bytes{read_file(argv[2])};
  binary_reader file{bytes};
  auto magic{file.take(8)};
  if(std::string(magic.begin(), magic.end()) != "ESWCHG01") file.fail("fixture version");
  auto count{file.integer<uint32_t>()};
  unsigned frames{0};
  unsigned failed{0};
  for(uint32_t i{0}; i < count; ++i) {
    auto name{file.cstring()};
    rectangle bounds{file.integer<int32_t>(), file.integer<int32_t>(), file.integer<int32_t>(), file.integer<int32_t>()};
    weapon_instruments instrument{resources, read_cockpit_layout(resources.read("simvol0", "gau/" + name + ".GAU"))};
    framebuffer frame;
    frame.clear(123);
    auto steps{file.integer<uint32_t>()};
    for(uint32_t j{0}; j < steps; ++j) {
      weapon_instrument_state state;
      state.value = file.integer<int32_t>();
      state.requested_charge = file.integer<int32_t>();
      state.selected = true;
      state.included = true;
      state.ready = true;
      auto full{file.integer<uint8_t>()};
      auto background{file.integer<uint8_t>()};
      auto expected{file.integer<uint32_t>()};
      if(full) {
        frame.clear(background);
        instrument.repaint();
      }
      instrument.draw_retained(frame, 0, weapon_instrument_kind::energy, "LAS100", state);
      std::vector<uint8_t> crop;
      for(int y{bounds.top}; y <= bounds.bottom; ++y) {
        for(int x{bounds.left}; x <= bounds.right; ++x) {
          crop.push_back(frame.pixels()[y * screen_width + x]);
        }
      }
      if(auto actual{crc32(crop)}; actual != expected) {
        if(failed++ < 16) std::cerr << name << " step " << j << " full " << static_cast<int>(full) << " charge " << state.value << " requested " << state.requested_charge << ": " << actual << " != " << expected << '\n';
      }
      ++frames;
    }
  }
  file.require_end();
  if(failed) throw std::runtime_error{std::to_string(failed) + " retained charge frames differ"};
  std::cout << frames << " original cold/full/incremental charge frames agree\n";
  return EXIT_SUCCESS;
} catch(std::exception const &e) {
  std::cerr << e.what() << '\n';
  return EXIT_FAILURE;
}
