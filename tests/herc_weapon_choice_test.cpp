#include <cstdlib>
#include <iostream>
#include "resources/resource_store.h"
#include "resources/herc_definition.h"
#include "simulation/herc_weapon_selection.h"
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
  if(std::string(magic.begin(), magic.end()) != "ESHCHO01") file.fail("fixture version");
  auto const definitions{read_weapon_catalogue(resources.read("simvol0", "dat/WEAPONS.DAT"))};
  auto const templates{read_model(resources.read("simvol0", "dts/MECHWPNS.DTS"))};
  auto const banks{file.integer<uint16_t>()}; unsigned int decisions{0};
  for(unsigned int row{0}; row != banks; ++row) {
    auto const raw_name{file.take(file.integer<uint16_t>())}; std::string const name(raw_name.begin(), raw_name.end());
    auto const definition{read_herc_definition(resources, name)};
    auto const equipment{read_words(file)}, values{read_words(file)};
    weapon_bank const pristine{definitions, templates, definition.mounts, equipment, values};
    auto const trials{file.integer<uint16_t>()};
    for(unsigned int trial{0}; trial != trials; ++trial) {
      auto const label{std::to_string(row) + '/' + std::to_string(trial)};
      auto bank{pristine}; auto const slots{file.integer<uint16_t>()};
      if(slots != bank.slots().size()) file.fail(label + " bank slots");
      for(unsigned int i{0}; i != slots; ++i) {
        bool const present{file.integer<uint8_t>() != 0};
        if(present != bank.slots()[i].has_value()) file.fail(label + " weapon presence");
        if(!present) continue;
        auto &weapon{bank.at(i)};
        weapon.controls().disabled = file.integer<uint8_t>() != 0; weapon.controls().refire_remaining = file.integer<int16_t>();
        auto const setting{file.integer<int16_t>()}, charge{file.integer<int16_t>()}, rounds{file.integer<int16_t>()};
        if(auto energy{weapon.energy()}) { energy->setting = setting; energy->charge = charge; }
        if(auto ammunition{weapon.ammunition()}) ammunition->rounds = rounds;
        bool const excluded{file.integer<uint8_t>() != 0}; auto const channel{file.integer<uint8_t>()}; bool const ready{file.integer<uint8_t>() != 0};
        if(weapon_excluded_from_combat(weapon) != excluded || weapon_lock_channel(weapon, definitions).value_or(5) != channel || weapon.ready(definitions.weapons.at(static_cast<size_t>(weapon.definition))) != ready) file.fail(label + " virtual weapon properties");
      }
      auto const kind{static_cast<combat_actor_kind>(file.integer<uint8_t>())}; auto const distance{file.integer<int32_t>()}; auto const facing{file.integer<int16_t>()};
      std::array<int16_t, 2> const shields{file.integer<int16_t>(), file.integer<int16_t>()};
      herc_weapon_selection_state state; state.aim_tolerance = file.integer<int16_t>();
      for(auto &v : state.locks) v = file.integer<uint8_t>() != 0;
      state.detectable = file.integer<uint8_t>() != 0; state.unavailable = file.integer<uint8_t>() != 0;
      state.ecm_remaining = file.integer<int16_t>(); auto delay{file.integer<int16_t>()}; random_state random{read_random(file)};
      auto const selected{file.integer<int16_t>()}; bool const detectable{file.integer<uint8_t>() != 0}, unavailable{file.integer<uint8_t>() != 0}; auto const final_delay{file.integer<int16_t>()};
      std::vector<uint8_t> expected(file.integer<uint8_t>()), actual;
      for(auto &v : expected) v = file.integer<uint8_t>(); auto const expected_rng{read_random(file)};
      auto const result{select_herc_weapon(bank, definitions, state, kind, distance, facing, random, delay, [&](bool front){ actual.push_back(static_cast<uint8_t>(front)); return shields[front ? 0 : 1]; }, [&]{ actual.push_back(2); })};
      if((result ? static_cast<int>(*result) : -1) != selected || state.detectable != detectable || state.unavailable != unavailable || delay != final_delay || actual != expected) file.fail(label + " weapon choice/state");
      auto const &rng{random.snapshot()};
      if(rng.words != expected_rng.words || rng.first != expected_rng.first || rng.second != expected_rng.second) file.fail(label + " RNG");
      ++decisions;
    }
  }
  file.require_end(); if(banks != 34 || decisions != 2448) file.fail("weapon choice coverage");
  std::cout << decisions << " original HERC weapon choices agree\n";
  return EXIT_SUCCESS;
} catch(std::exception const &error) { std::cerr << error.what() << '\n'; return EXIT_FAILURE; }
