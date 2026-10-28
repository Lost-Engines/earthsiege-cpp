#include <cstdlib>
#include <fstream>
#include <iostream>
#include "render/scanner_display.h"

int main(int argc, char **argv) try {
  using namespace earthsiege;
  if(argc != 3) return EXIT_FAILURE;
  resource_store resources{argv[1]}; scanner_display display{resources};
  auto const source{read_file(argv[2])}; binary_reader file{source};
  auto const magic{file.take(8)};
  if(std::string(magic.begin(), magic.end()) != "ESSCAN01") file.fail("fixture version");
  auto const count{file.integer<uint32_t>()};
  for(uint32_t i{}; i < count; ++i) {
    rectangle const bounds{file.integer<int32_t>(), file.integer<int32_t>(), file.integer<int32_t>(), file.integer<int32_t>()};
    scanner_display_state state;
    state.player_position = {file.integer<int32_t>(), file.integer<int32_t>()};
    state.heading = file.integer<uint16_t>(); state.ticks = file.integer<uint32_t>();
    state.range = file.integer<int32_t>(); state.radar_active = file.integer<uint8_t>();
    auto const contacts{file.integer<uint16_t>()};
    for(unsigned int j{}; j < contacts; ++j) {
      scanner_contact contact;
      contact.kind = static_cast<combat_actor_kind>(file.integer<int16_t>()); contact.side = file.integer<uint8_t>();
      contact.position = {file.integer<int32_t>(), file.integer<int32_t>()};
      contact.last_position = {file.integer<int32_t>(), file.integer<int32_t>()};
      auto const flags{file.integer<uint8_t>()};
      contact.active = flags & 1; contact.detected = flags & 2;
      contact.last_position_unavailable = flags & 4; contact.selected = flags & 8;
      state.contacts.push_back(contact);
    }
    auto const expected{file.integer<uint32_t>()};
    framebuffer frame; frame.clear(123); display.draw(frame, bounds, state);
    if(crc32(frame.pixels()) != expected) {
      std::ofstream output{"/tmp/es-scanner-difference.raw", std::ios::binary};
      output.write(reinterpret_cast<char const *>(frame.pixels().data()), frame.pixels().size());
      file.fail("scanner display paint differs at " + std::to_string(i));
    }
  }
  file.require_end(); std::cout << count << " original scanner frames match\n";
  return EXIT_SUCCESS;
} catch(std::exception const &e) { std::cerr << e.what() << '\n'; return EXIT_FAILURE; }
