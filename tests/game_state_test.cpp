#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include "resources/career_index.h"
#include "shell/game_state.h"
using earthsiege::binary_reader;
using earthsiege::bytes;
using earthsiege::game_state;
using earthsiege::normalise_path;
using earthsiege::random_state;
using earthsiege::read_career_index;
using earthsiege::read_file;
using earthsiege::resource_store;
using earthsiege::write_career_index;
using earthsiege::write_career_state;
using earthsiege::write_mission_counters;
using earthsiege::write_mission_script;
using earthsiege::write_player_roster;
namespace {
void require(bool ok, std::string const &what) {
  if(!ok) throw std::runtime_error{what};
}
bytes blob(binary_reader &f) {
  return f.take(f.integer<uint32_t>());
}
void compare(bytes actual, bytes expected, std::string const &what) {
  if(std::ranges::equal(actual, expected)) return;
  auto diff{std::ranges::mismatch(actual, expected)};
  std::cerr << what << " sizes " << actual.size() << '/' << expected.size() << " difference " << diff.in1 - actual.begin() << '\n';
  throw std::runtime_error{"original shell state mismatch"};
}
random_state::state random(binary_reader &f) {
  random_state::state s;
  for(auto &w : s.words) w = f.integer<uint16_t>();
  s.first = f.integer<uint8_t>();
  s.second = f.integer<uint8_t>();
  return s;
}
void compare(random_state::state const &a, random_state::state const &b) {
  require(a.words == b.words && a.first == b.first && a.second == b.second, "original shell RNG mismatch");
}
} // anonymous namespace
int main(int argc, char **argv) try {
  if(argc != 3) return EXIT_FAILURE;
  resource_store resources{argv[1]};
  auto data{read_file(argv[2])};
  binary_reader f{data};
  auto m{f.take(8)};
  require(std::string(m.begin(), m.end()) == "ESSHL001", "fixture version");
  auto count{f.integer<uint32_t>()};
  for(unsigned i{0}; i < count; ++i) {
    auto seed{f.integer<int16_t>()};
    auto mode{f.integer<int16_t>()};
    auto category{f.integer<int16_t>()};
    auto battle{f.integer<int16_t>()};
    auto name{f.cstring()};
    auto script{blob(f)};
    auto counters{blob(f)};
    auto player{blob(f)};
    auto crew{blob(f)};
    auto final_player{blob(f)};
    auto before{random(f)};
    auto after{random(f)};
    game_state game{resources, seed};
    if(mode) {
      game.new_campaign(battle);
    } else {
      game.prepare_single(category, {});
    }
    require(normalise_path("msn/" + game.mission_name() + ".msn") == normalise_path(name), "original random mission choice");
    compare(write_mission_script(game.prepared().compiled.script), script, name + " script");
    compare(write_mission_counters(game.prepared().counters), counters, name + " counters");
    compare(write_player_roster(game.player()), player, name + " prepared player");
    if(mode) compare(write_career_state(game.crew()), crew, name + " selected crew");
    compare(game.random_snapshot(), before);
    game.update_roster();
    compare(write_player_roster(game.player()), final_player, name + " final player");
    compare(game.random_snapshot(), after);
  }
  f.require_end();
  auto const original_index{resources.read("shell0", "car/names.log")};
  compare(write_career_index(read_career_index(original_index)), original_index, "supplied career index");
  std::cout << count << " original complete shell choices, crew activation and final roster exports match\n";
  return EXIT_SUCCESS;
} catch(std::exception const &e) {
  std::cerr << e.what() << '\n';
  return EXIT_FAILURE;
}
