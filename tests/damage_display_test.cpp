#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include "render/damage_display.h"

int main(int argc, char **argv) try {
  using namespace earthsiege;
  if(argc != 3) return EXIT_FAILURE;
  resource_store resources{argv[1]};
  auto const source{read_file(argv[2])}; binary_reader file{source};
  auto const magic{file.take(8)};
  if(std::string(magic.begin(), magic.end()) != "ESDAMAGE") file.fail("fixture version");
  auto const count{file.integer<uint32_t>()};
  std::string chassis; std::unique_ptr<damage_display> display;
  for(uint32_t i{}; i < count; ++i) {
    auto const name{file.cstring()}, label{file.cstring()};
    if(chassis != name) { chassis = name; display = std::make_unique<damage_display>(resources); }
    rectangle const bounds{file.integer<int32_t>(), file.integer<int32_t>(), file.integer<int32_t>(), file.integer<int32_t>()};
    damage_display_state state;
    state.kind = static_cast<damage_display_kind>(file.integer<int16_t>());
    state.model = file.integer<int16_t>(); state.total_damage = file.integer<int16_t>();
    auto const flags{file.integer<uint8_t>()};
    state.friendly = flags & 1; state.player = flags & 2; state.dead = flags & 4; state.shield_warning = flags & 8;
    for(auto &value : state.armour) value = file.integer<int16_t>();
    for(auto &value : state.internal) value = file.integer<int16_t>();
    auto const expected{file.integer<uint32_t>()};
    framebuffer frame; frame.clear(123); display->draw(frame, bounds, state);
    if(crc32(frame.pixels()) != expected) {
      std::ofstream output{"/tmp/es-damage-display-difference.raw", std::ios::binary};
      output.write(reinterpret_cast<char const *>(frame.pixels().data()), frame.pixels().size());
      file.fail(name + " " + label + " damage display paint differs at " + std::to_string(i));
    }
  }
  file.require_end(); std::cout << count << " original damage display frames match\n";
  return EXIT_SUCCESS;
} catch(std::exception const &e) { std::cerr << e.what() << '\n'; return EXIT_FAILURE; }
