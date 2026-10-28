#include <cstdlib>
#include <iostream>
#include "resources/resource_store.h"
#include "resources/herc_definition.h"
#include "simulation/herc_fire_control.h"
namespace {
using namespace earthsiege;
random_state::state read_random(binary_reader &file) {
  random_state::state state; for(auto &v : state.words) v = file.integer<uint16_t>();
  state.first = file.integer<uint8_t>(); state.second = file.integer<uint8_t>(); return state;
}
std::vector<int16_t> read_words(binary_reader &file) {
  std::vector<int16_t> words(file.integer<uint16_t>()); for(auto &v : words) v = file.integer<int16_t>(); return words;
}
std::optional<size_t> slot(int16_t value) { return value < 0 ? std::nullopt : std::make_optional(static_cast<size_t>(value)); }
}
int main(int argc, char **argv) try {
  if(argc != 3) return EXIT_FAILURE;
  resource_store resources{argv[1]}; auto const bytes{read_file(argv[2])}; binary_reader file{bytes}; auto const magic{file.take(8)};
  if(std::string(magic.begin(), magic.end()) != "ESHFI001") file.fail("fixture version");
  auto const definitions{read_weapon_catalogue(resources.read("simvol0", "dat/WEAPONS.DAT"))};
  auto const templates{read_model(resources.read("simvol0", "dts/MECHWPNS.DTS"))};
  auto const banks{file.integer<uint16_t>()}; size_t decisions{0}, shots{0};
  for(unsigned int row{0}; row != banks; ++row) {
    auto const name{file.cstring()}; auto const definition{read_herc_definition(resources, name)};
    auto const equipment{read_words(file)}, values{read_words(file)};
    weapon_bank const pristine{definitions, templates, definition.mounts, equipment, values};
    auto const trials{file.integer<uint16_t>()};
    for(unsigned int trial{0}; trial != trials; ++trial) {
      auto const label{std::to_string(row) + '/' + std::to_string(trial)};
      auto bank{pristine}; auto const slots{file.integer<uint16_t>()}; if(slots != bank.slots().size()) file.fail(label + " slots");
      for(unsigned int i{0}; i != slots; ++i) {
        bool const present{file.integer<uint8_t>() != 0}; if(present != bank.slots()[i].has_value()) file.fail(label + " presence");
        if(!present) continue;
        auto &w{bank.at(i)};
        w.controls().disabled = file.integer<uint8_t>() != 0; w.controls().refire_remaining = file.integer<int16_t>();
        auto const setting{file.integer<int16_t>()}, charge{file.integer<int16_t>()}, rounds{file.integer<int16_t>()};
        if(auto energy{w.energy()}) { energy->setting = setting; energy->charge = charge; }
        if(auto ammunition{w.ammunition()}) ammunition->rounds = rounds;
      }
      herc_fire_state state; state.retained_weapon = slot(file.integer<int16_t>()); auto const chosen{slot(file.integer<int16_t>())};
      state.skip_selection = file.integer<uint8_t>() != 0;
      herc_accuracy const accuracy{file.integer<uint8_t>() != 0, file.integer<uint8_t>() != 0, file.integer<uint8_t>()};
      vec3<int32_t> const position{file.integer<int32_t>(), file.integer<int32_t>(), file.integer<int32_t>()};
      herc_fire_target target; target.point = {file.integer<int32_t>(), file.integer<int32_t>(), file.integer<int32_t>()}; target.heading = file.integer<uint16_t>(); target.relative_facing = file.integer<int16_t>();
      auto const speed{file.integer<int16_t>()}; vec3<int16_t> const errors{file.integer<int16_t>(), file.integer<int16_t>(), file.integer<int16_t>()};
      random_state random{read_random(file)};
      std::vector<std::array<int32_t, 4>> expected(file.integer<uint8_t>()), actual;
      for(auto &event : expected) for(auto &v : event) v = file.integer<int32_t>();
      auto const final_retained{slot(file.integer<int16_t>())}; bool const final_skip{file.integer<uint8_t>() != 0}; auto const final_rng{read_random(file)};
      aim_and_fire_herc(state, bank, definitions, position, target, accuracy, random, {
        .select_weapon{[&](int32_t distance, int16_t facing){ actual.push_back({0, distance, facing, 0}); return chosen; }},
        .target_speed{[&]{ actual.push_back({1, 0, 0, 0}); return speed; }},
        .aim{[&](vec3<int32_t> const &point){ actual.push_back({2, point.x, point.y, point.z}); return vec3<int16_t>{errors}; }},
        .fire{[&](size_t selected){ actual.push_back({3, static_cast<int32_t>(selected), 0, 0}); ++shots; }},
      });
      if(actual != expected) {
        for(auto const &event : actual) std::cerr << "actual " << event[0] << ',' << event[1] << ',' << event[2] << ',' << event[3] << '\n';
        for(auto const &event : expected) std::cerr << "expected " << event[0] << ',' << event[1] << ',' << event[2] << ',' << event[3] << '\n';
        file.fail(label + " firing events");
      }
      if(state.retained_weapon != final_retained || state.skip_selection != final_skip) file.fail(label + " retained weapon/skip");
      auto const &rng{random.snapshot()}; if(rng.words != final_rng.words || rng.first != final_rng.first || rng.second != final_rng.second) file.fail(label + " RNG");
      ++decisions;
    }
  }
  file.require_end(); if(banks != 32 || decisions != 3072 || shots == 0) file.fail("fire coverage");
  std::cout << decisions << " original HERC fire decisions and " << shots << " firing requests agree\n";
  return EXIT_SUCCESS;
} catch(std::exception const &error) { std::cerr << error.what() << '\n'; return EXIT_FAILURE; }
