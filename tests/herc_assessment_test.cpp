#include <cstdlib>
#include <iostream>
#include "resources/input_tape.h"
#include "resources/resource_store.h"
#include "simulation/mission_session.h"
#include "simulation/herc_assessment.h"

namespace {
using namespace earthsiege;
random_state::state read_random(binary_reader &file) {
  random_state::state state; for(auto &v : state.words) v = file.integer<uint16_t>();
  state.first = file.integer<uint8_t>(); state.second = file.integer<uint8_t>(); return state;
}
std::vector<int16_t> read_words(binary_reader &file) {
  std::vector<int16_t> words(file.integer<uint16_t>()); for(auto &v : words) v = file.integer<int16_t>(); return words;
}
}
int main(int argc, char **argv) try {
  if(argc != 3) return EXIT_FAILURE;
  resource_store resources{argv[1]}; auto const bytes{read_file(argv[2])}; binary_reader file{bytes}; auto const magic{file.take(8)};
  if(std::string(magic.begin(), magic.end()) != "ESHASS01") file.fail("fixture version");
  auto const weapons{read_weapon_catalogue(resources.read("simvol0", "dat/WEAPONS.DAT"))};
  auto const worlds{file.integer<uint16_t>()}; unsigned int ratings{0};
  for(unsigned int world{0}; world != worlds; ++world) {
    auto const demo{file.integer<uint16_t>()}; auto const initial{read_random(file)};
    auto const tape{read_input_tape(read_file(resources.root() / "TAPES" / ("DEMO" + std::to_string(demo) + ".TAP")))};
    auto const script{read_mission_script(tape.starting_files[std::to_underlying(tape_file::script)])};
    auto const roster{read_player_roster(tape.starting_files[std::to_underlying(tape_file::player)])};
    mission_session session{resources, script, roster, random_state{initial}};
    auto const actors{session.hercs()}; auto const count{file.integer<uint16_t>()};
    for(unsigned int a{0}; a != count; ++a) {
      auto const index{file.integer<uint16_t>()}, cases{file.integer<uint16_t>()};
      auto const &actor{actors.at(index).actor};
      for(unsigned int trial{0}; trial != cases; ++trial) {
        auto damage{actor.damage()}; damage.internal = read_words(file); damage.armour = read_words(file);
        auto const disabled{read_words(file)}, ammunition{read_words(file)};
        damage.present = read_words(file);
        auto const rating{file.integer<int16_t>()}; auto const collapsed{file.integer<uint8_t>() != 0};
        random_state random{read_random(file)}; auto const selected{file.integer<int16_t>()}; auto const expected_rng{read_random(file)};
        auto bank{actor.weapons()};
        for(size_t i{0}; i != bank.slots().size(); ++i) if(bank.slots()[i]) {
          auto &weapon{bank.at(i)}; weapon.controls().disabled = disabled.at(i) != 0;
          if(auto supply{weapon.ammunition()}) supply->rounds = ammunition.at(i);
        }
        auto const label{std::to_string(demo) + '/' + std::to_string(index) + '/' + std::to_string(trial)};
        auto const actual{herc_combat_rating(actor.profile(), actor.definition().damage, damage, bank, weapons)};
        if(actual != rating) file.fail(label + " rating " + std::to_string(actual) + " expected " + std::to_string(rating));
        component_damage components{actor.definition().damage, damage};
        if(select_herc_aimed_component(components, bank.slots().size(), damage.present, collapsed, random) != selected) file.fail(label + " aimed component");
        auto const &rng{random.snapshot()};
        if(rng.words != expected_rng.words || rng.first != expected_rng.first || rng.second != expected_rng.second) file.fail(label + " RNG");
        ++ratings;
      }
    }
  }
  auto const retreats{file.integer<uint16_t>()};
  for(unsigned int row{0}; row != retreats; ++row) {
    herc_retreat_inputs input; input.caution = file.integer<int16_t>();
    for(auto &v : input.internal_damage) { v = file.integer<int16_t>(); if(v < 0) v = 256; }
    input.incapacitated = file.integer<uint8_t>() != 0; input.target_kind = static_cast<combat_actor_kind>(file.integer<uint8_t>());
    input.target_can_retaliate = file.integer<uint8_t>() != 0; input.attackers = file.integer<int16_t>(); input.strength = file.integer<int16_t>();
    auto const total{file.integer<int32_t>()}; auto tolerance{file.integer<int16_t>()};
    auto const result{file.integer<uint8_t>() != 0}; auto const expected_tolerance{file.integer<int16_t>()};
    auto const transition{static_cast<herc_retreat_action>(file.integer<uint8_t>())}; auto const expected_reads{file.integer<uint8_t>()};
    unsigned int reads{0}; auto const actual{assess_herc_retreat(input, tolerance, [&]{ ++reads; return total; })};
    if(actual != transition || result != (actual != herc_retreat_action::none) || tolerance != expected_tolerance || reads != expected_reads) file.fail(std::to_string(row) + " retreat decision");
  }
  file.require_end(); if(ratings != 1848 || retreats != 2048) file.fail("assessment coverage");
  std::cout << ratings << " combat ratings and component choices, " << retreats << " retreat decisions agree\n";
  return EXIT_SUCCESS;
} catch(std::exception const &error) { std::cerr << error.what() << '\n'; return EXIT_FAILURE; }
