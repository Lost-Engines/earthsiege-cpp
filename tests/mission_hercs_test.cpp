#include <cstdlib>
#include <iostream>
#include <string>
#include "resources/input_tape.h"
#include "resources/resource_store.h"
#include "simulation/mission_session.h"

namespace {
std::vector<int16_t> shorts(earthsiege::binary_reader &file) {
  std::vector<int16_t> values(file.integer<uint16_t>());
  for(auto &v : values) v = file.integer<int16_t>();
  return values;
}
earthsiege::random_state::state random(earthsiege::binary_reader &file) {
  earthsiege::random_state::state result;
  for(auto &v : result.words) v = file.integer<uint16_t>();
  result.first = file.integer<uint8_t>(); result.second = file.integer<uint8_t>();
  return result;
}
}

int main(int const argc, char **argv) try {
  if(argc != 3) return EXIT_FAILURE;
  earthsiege::resource_store resources{argv[1]};
  auto const bytes{earthsiege::read_file(argv[2])}; earthsiege::binary_reader file{bytes};
  auto const magic{file.take(8)};
  if(std::string(magic.begin(), magic.end()) != "ESROSTR1") file.fail("fixture version");
  auto const worlds{file.integer<uint16_t>()}; size_t actors{0};
  for(unsigned int world{0}; world != worlds; ++world) {
    auto const demo{file.cstring()}; auto const initial_random{random(file)}; auto const expected_random{random(file)};
    auto const tape{earthsiege::read_input_tape(earthsiege::read_file(resources.root() / "TAPES" / (demo + ".TAP")))};
    auto const script{earthsiege::read_mission_script(tape.starting_files[std::to_underlying(earthsiege::tape_file::script)])};
    auto const roster{earthsiege::read_player_roster(tape.starting_files[std::to_underlying(earthsiege::tape_file::player)])};
    earthsiege::mission_session session{resources, script, roster, earthsiege::random_state{initial_random}};
    auto const actual{session.hercs()}; auto const count{file.integer<uint16_t>()};
    if(count != actual.size()) file.fail(demo + " roster size");
    for(size_t i{0}; i != actual.size(); ++i) {
      auto const label{demo + '/' + std::to_string(i)}; auto const &member{actual[i]}; auto const &actor{member.actor};
      auto const source{file.integer<uint16_t>()}; auto const group{file.integer<uint16_t>()}; auto const side{file.integer<uint8_t>()};
      bool const player_roster{file.integer<uint8_t>() != 0}, player{file.integer<uint8_t>() != 0};
      vec3<int32_t> const position{file.integer<int32_t>(), file.integer<int32_t>(), file.integer<int32_t>()};
      vec3<uint16_t> const angles{file.integer<uint16_t>(), file.integer<uint16_t>(), file.integer<uint16_t>()};
      if(member.source != source || member.group != group || member.side != side || member.player_roster != player_roster || member.player != player) file.fail(label + " identity/group");
      if(actor.body().movement().position != position || actor.body().movement().angles != angles) file.fail(label + " placement");
      auto const internal{shorts(file)}, armour{shorts(file)}, present{shorts(file)}, cells{shorts(file)};
      auto const &damage{actor.damage()};
      if(damage.internal != internal || damage.armour != armour || damage.present != present || damage.cells != cells) file.fail(label + " starting damage");
      earthsiege::damage_consequences const consequences{file.integer<uint8_t>() != 0, file.integer<uint8_t>() != 0, file.integer<uint8_t>() != 0, file.integer<uint8_t>() != 0, file.integer<uint8_t>() != 0, file.integer<uint8_t>() != 0};
      bool const salvage_excluded{file.integer<uint8_t>() != 0}, collapsed{file.integer<uint8_t>() != 0}, detectable{file.integer<uint8_t>() != 0}, waiting{file.integer<uint8_t>() != 0};
      if(damage.consequences != consequences || actor.allows_salvage() == salvage_excluded || actor.body().movement().collapsed != collapsed || actor.impacts().detectable != detectable || member.waiting != waiting) file.fail(label + " actor flags");
      ++actors;
    }
    auto const &rng{session.random_snapshot()};
    if(rng.words != expected_random.words || rng.first != expected_random.first || rng.second != expected_random.second) file.fail(demo + " construction randomness");
  }
  file.require_end();
  if(worlds != 4 || actors != 56) file.fail("mission roster coverage");
  std::cout << actors << " complete original mission HERC starts agree across " << worlds << " recorded missions\n";
  return EXIT_SUCCESS;
} catch(std::exception const &error) { std::cerr << error.what() << '\n'; return EXIT_FAILURE; }
