#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <string>
#include "resources/input_tape.h"
#include "resources/resource_store.h"
#include "simulation/mission_session.h"

int main(int const argc, char **argv) try {
  using namespace earthsiege;
  if(argc != 3) return EXIT_FAILURE;
  resource_store resources{argv[1]}; auto const bytes{read_file(argv[2])}; binary_reader file{bytes}; auto const magic{file.take(8)};
  if(std::string(magic.begin(),magic.end()) != "ESFSTART") file.fail("fixture version");
  auto const shorts{[&]{ std::vector<int16_t> v(file.integer<uint16_t>()); for(auto &x : v) x = file.integer<int16_t>(); return v; }};
  auto const worlds{file.integer<uint16_t>()}; unsigned int count{0};
  for(unsigned int world{0}; world != worlds; ++world) {
    auto const demo{file.cstring()}; auto const tape{read_input_tape(read_file(resources.root() / "TAPES" / (demo + ".TAP")))};
    auto const script{read_mission_script(tape.starting_files[std::to_underlying(tape_file::script)])};
    auto const roster{read_player_roster(tape.starting_files[std::to_underlying(tape_file::player)])};
    mission_session session{resources,script,roster}; auto const actors{session.flying_actors()}; auto const expected{file.integer<uint16_t>()};
    if(actors.size() != expected) file.fail(demo + " flyer count");
    for(auto const &member : actors) {
      auto const source{file.integer<uint16_t>()}, group{file.integer<uint16_t>()}; auto const side{file.integer<uint8_t>()};
      bool const waiting{file.integer<uint8_t>() != 0}, dead{file.integer<uint8_t>() != 0}, detectable{file.integer<uint8_t>() != 0};
      vec3<int32_t> const position{file.integer<int32_t>(),file.integer<int32_t>(),file.integer<int32_t>()};
      vec3<uint16_t> const angles{file.integer<uint16_t>(),file.integer<uint16_t>(),file.integer<uint16_t>()};
      vec3<int16_t> const velocity{file.integer<int16_t>(),file.integer<int16_t>(),file.integer<int16_t>()}, rates{file.integer<int16_t>(),file.integer<int16_t>(),file.integer<int16_t>()};
      auto const internal{shorts()}, armour{shorts()}, present{shorts()}, cells{shorts()};
      auto const &actor{member.actor}; auto const &damage{actor.damage()};
      if(member.source != source || member.group != group || member.side != side || member.waiting != waiting) file.fail(demo + " flyer identity");
      if(actor.dead() != dead || actor.is_detectable() != detectable || actor.placement().translation != position || actor.angles() != angles || actor.local_velocity() != velocity || actor.angular_velocity() != rates) file.fail(demo + " flyer state");
      if(damage.internal != internal || damage.armour != armour || damage.present != present || !std::ranges::equal(actor.instance().cell_frames(),cells)) file.fail(demo + " flyer damage");
      ++count;
    }
  }
  file.require_end(); if(worlds != 4 || count != 18) file.fail("mission flyer coverage");
  std::cout << count << " original mission flyer starts agree\n"; return EXIT_SUCCESS;
} catch(std::exception const &error) { std::cerr << error.what() << '\n'; return EXIT_FAILURE; }
