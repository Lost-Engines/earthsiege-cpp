#include <cstdlib>
#include <fstream>
#include <iostream>
#include "render/weapon_instruments.h"
#include "resources/input_tape.h"
#include "shell/cockpit_radio.h"
using earthsiege::binary_reader;
using earthsiege::cockpit_radio;
using earthsiege::crc32;
using earthsiege::framebuffer;
using earthsiege::mission_session;
using earthsiege::random_state;
using earthsiege::read_cockpit_layout;
using earthsiege::read_file;
using earthsiege::read_input_tape;
using earthsiege::read_mission_script;
using earthsiege::read_player_roster;
using earthsiege::rectangle;
using earthsiege::resource_store;
using earthsiege::tape_file;
using earthsiege::weapon_instrument_kind;
using earthsiege::weapon_instrument_state;
using earthsiege::weapon_instruments;
int main(int argc, char **argv) try {
  if(argc != 3) return EXIT_FAILURE;
  resource_store resources{argv[1]};
  auto raw{read_file(argv[2])};
  binary_reader file{raw};
  auto magic{file.take(8)};
  if(std::string(magic.begin(), magic.end()) != "ESWERAS1") file.fail("fixture version");
  auto tape{read_input_tape(read_file(resources.root() / "TAPES/DEMO1.TAP"))};
  mission_session session{resources, read_mission_script(tape.starting_files[std::to_underlying(tape_file::script)]), read_player_roster(tape.starting_files[std::to_underlying(tape_file::player)])};
  auto const layout{read_cockpit_layout(resources.read("simvol0", "gau/HERCULES.GAU"))};
  auto const bounds{layout.bounds(0x694)};
  auto count{file.integer<uint32_t>()};
  size_t total{0};
  for(unsigned k{0}; k < count; ++k) {
    auto kind{static_cast<weapon_instrument_kind>(file.integer<uint32_t>())};
    auto steps{file.integer<uint32_t>()};
    random_state random{17};
    cockpit_radio radio{resources, layout, session, random};
    weapon_instruments weapon{resources, layout};
    framebuffer backing;
    backing.clear(123);
    weapon.draw_retained(backing, 3, kind, "TEST", {});
    weapon.repaint();
    weapon.painted_region = [&](rectangle const &b){radio.overpaint(b);};
    for(unsigned i{0}; i < steps; ++i) {
      weapon_instrument_state state;
      state.value = file.integer<int32_t>();
      state.disabled = file.integer<uint8_t>();
      state.selected = file.integer<uint8_t>();
      state.included = file.integer<uint8_t>();
      state.ready = file.integer<uint8_t>();
      bool erase{file.integer<uint8_t>() != 0};
      auto expected{file.integer<uint32_t>()};
      framebuffer frame{backing};
      weapon.draw_retained(frame, 3, kind, "TEST", state);
      if(erase) radio.repaint();
      radio.draw(frame);
      std::vector<uint8_t> crop;
      for(int y{bounds.top}; y <= bounds.bottom; ++y) {
        for(int x{bounds.left}; x <= bounds.right; ++x) crop.push_back(frame.pixels()[y * 320 + x]);
      }
      if(crc32(crop) != expected) {
        std::ofstream debug{"/tmp/earthsiege-weapon-message.raw", std::ios::binary};
        debug.write(reinterpret_cast<char const *>(crop.data()), static_cast<std::streamsize>(crop.size()));
        debug.close();
        file.fail("retained weapon/message overlap kind " + std::to_string(k) + " frame " + std::to_string(i));
      }
      ++total;
    }
  }
  file.require_end();
  std::cout << total << " original retained weapon/message overlap rasters match\n";
  return EXIT_SUCCESS;
} catch(std::exception const &e) {
  std::cerr << e.what() << '\n';
  return EXIT_FAILURE;
}
