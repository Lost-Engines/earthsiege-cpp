#include <array>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include "simulation/component_damage.h"

namespace {

std::vector<int16_t> shorts(earthsiege::binary_reader &file) {
  std::vector<int16_t> values(file.integer<uint16_t>());
  for(auto &value : values) value = file.integer<int16_t>();
  return values;
}

earthsiege::damage_state state(earthsiege::binary_reader &file) {
  earthsiege::damage_state result;
  result.internal = shorts(file);
  result.armour = shorts(file);
  result.present = shorts(file);
  result.cells = shorts(file);
  auto &flags{result.consequences};
  flags.dead = file.integer<uint8_t>() != 0;
  flags.disabled = file.integer<uint8_t>() != 0;
  flags.movement_light = file.integer<uint8_t>() != 0;
  flags.movement_heavy = file.integer<uint8_t>() != 0;
  flags.reactor_light = file.integer<uint8_t>() != 0;
  flags.reactor_heavy = file.integer<uint8_t>() != 0;
  return result;
}

void check(earthsiege::damage_state const &a, earthsiege::damage_state const &b, std::string const &label) {
  if(a.internal != b.internal || a.armour != b.armour || a.present != b.present || a.cells != b.cells || a.consequences != b.consequences) throw std::runtime_error{label + ": damage state differs"};
}

} // namespace

int main(int const argc, char **argv) try {
  if(argc != 2) return EXIT_FAILURE;
  auto const bytes{earthsiege::read_file(argv[1])};
  earthsiege::binary_reader file{bytes};
  auto const magic{file.take(8)};
  if(std::string(magic.begin(), magic.end()) != "ESDMG001") file.fail("fixture version");
  auto const accumulators{file.integer<uint16_t>()};
  for(unsigned int row{0}; row != accumulators; ++row) {
    auto amount{file.integer<int16_t>()};
    auto const capacity{file.integer<int16_t>()};
    auto current{file.integer<int16_t>()};
    auto const expected_current{file.integer<int16_t>()};
    auto const expected_amount{file.integer<int16_t>()};
    bool const expected_dead{file.integer<uint8_t>() != 0};
    bool const dead{earthsiege::accumulate_damage(amount, capacity, current)};
    if(dead != expected_dead || amount != expected_amount || current != expected_current) file.fail("damage accumulator " + std::to_string(row) + " differs");
  }
  auto const chassis{file.integer<uint16_t>()};
  unsigned int total{0};
  for(unsigned int row{0}; row != chassis; ++row) {
    auto const name{file.cstring()};
    auto const legs{file.integer<int16_t>()};
    auto const definition_bytes{file.integer<uint32_t>()};
    auto const definition{earthsiege::read_damage_definition(file.take(definition_bytes))};
    auto const slots{shorts(file)};
    auto const initial{state(file)};
    auto const traces{file.integer<uint16_t>()};
    for(unsigned int trace{0}; trace != traces; ++trace) {
      earthsiege::component_damage damage{definition, initial};
      auto const hits{file.integer<uint16_t>()};
      for(unsigned int hit{0}; hit != hits; ++hit) {
        auto const section{file.integer<int16_t>()};
        auto const amount{file.integer<int16_t>()};
        auto const draw_count{file.integer<uint16_t>()};
        std::vector<std::array<int16_t, 2>> draws;
        for(unsigned int i{0}; i != draw_count; ++i) draws.push_back({file.integer<int16_t>(), file.integer<int16_t>()});
        auto const expected_removed{shorts(file)};
        auto const expected_disabled{shorts(file)};
        auto const expected{state(file)};
        size_t consumed{0};
        auto const label{name + '/' + std::to_string(trace) + '/' + std::to_string(hit)};
        auto const removed{damage.hit(static_cast<size_t>(section), amount, legs, [&](int16_t const bound){
          if(consumed >= draws.size() || draws[consumed][0] != bound) file.fail(label + ": component choice bound differs");
          return draws[consumed++][1];
        })};
        if(consumed != draws.size()) file.fail(label + ": component draw count differs");
        check(damage.state(), expected, label);
        if(removed.size() != expected_removed.size()) file.fail(label + ": detachments differ");
        for(size_t i{0}; i != removed.size(); ++i) if(removed[i] != static_cast<size_t>(expected_removed[i])) file.fail(label + ": detachment order differs");
        if(slots.size() != expected_disabled.size()) file.fail("weapon-slot fixture differs");
        for(size_t i{0}; i != slots.size(); ++i) if(slots[i] >= 0 && static_cast<int16_t>(damage.state().present.at(static_cast<size_t>(19 + slots[i])) == 0) != expected_disabled[i]) file.fail(label + ": weapon disable differs");
        ++total;
      }
    }
  }
  file.require_end();
  if(chassis != 8 || total != 5106 || accumulators != 180) file.fail("damage reference coverage differs");
  std::cout << total << " original damage updates and " << accumulators << " signed accumulator cases agree" << std::endl;
  return EXIT_SUCCESS;
} catch(std::exception const &error) {
  std::cerr << error.what() << std::endl;
  return EXIT_FAILURE;
}
