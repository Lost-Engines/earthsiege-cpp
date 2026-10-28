#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>
#include "resources/binary_reader.h"
#include "simulation/player_targeting.h"

int main(int const argc, char **argv) try {
  if(argc != 2) return EXIT_FAILURE;
  auto const data{earthsiege::read_file(argv[1])}; earthsiege::binary_reader file{data};
  auto const magic{file.take(8)}; if(std::string(magic.begin(), magic.end()) != "ESPTAR01") file.fail("fixture version");
  auto const count{file.integer<uint32_t>()};
  auto const integer{[&]{return file.integer<int32_t>();}};
  auto const index{[&]() -> std::optional<size_t> {auto const value{integer()}; return value < 0 ? std::nullopt : std::optional<size_t>{static_cast<size_t>(value)};}};
  for(uint32_t row{0}; row != count; ++row) {
    earthsiege::cockpit_observer observer;
    observer.position = {integer(), integer(), integer()}; observer.heading = static_cast<uint16_t>(integer()); observer.torso = static_cast<int16_t>(integer()); observer.side = static_cast<uint8_t>(integer());
    observer.contact_range = integer() == 1 ? 30000 : 60000; observer.visible_range = integer();
    auto const previous{index()}, expected{index()}, stored{index()}; bool const changed{integer() != 0};
    std::vector<earthsiege::cockpit_target> actors(file.integer<uint16_t>());
    for(auto &actor : actors) {
      actor.position = {integer(), integer(), integer()}; actor.side = static_cast<uint8_t>(integer());
      actor.seen = integer() != 0; actor.known = integer() != 0; actor.dead = integer() != 0; actor.disabled = integer() != 0;
    }
    auto const actual{earthsiege::cycle_cockpit_target(actors, observer, previous)};
    if(actual != expected || actual != stored || (actual != previous) != changed) file.fail("cockpit target cycle differs at " + std::to_string(row));
  }
  file.require_end(); if(count != 4096) file.fail("target-cycle coverage differs");
  std::cout << count << " original cockpit target cycles agree\n"; return EXIT_SUCCESS;
} catch(std::exception const &e) { std::cerr << e.what() << '\n'; return EXIT_FAILURE; }
