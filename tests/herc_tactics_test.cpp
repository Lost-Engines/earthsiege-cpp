#include <array>
#include <cstdlib>
#include <iostream>
#include <tuple>
#include "resources/resource_store.h"
#include "simulation/herc_tactics.h"

namespace {
using namespace earthsiege;
herc_attack_state read_state(binary_reader &file) {
  return {file.integer<int16_t>(), file.integer<int16_t>(), file.integer<int16_t>(), file.integer<int16_t>(), file.integer<uint8_t>() != 0};
}
auto state_fields(herc_attack_state const &s) {
  return std::tuple{s.last_direction, s.steering_memory, s.reverse_remaining, s.damage_check_remaining, s.behind_target};
}
auto geometry_fields(herc_target_geometry const &g) {
  return std::tuple{g.bearing, g.bearing_error, g.bearing_magnitude, g.facing_error, g.facing_magnitude, g.distance, g.direction, g.near_distance, g.far_distance};
}
}
int main(int argc, char **argv) try {
  if(argc != 2) return EXIT_FAILURE;
  auto const bytes{read_file(argv[1])}; binary_reader file{bytes}; auto const magic{file.take(8)};
  if(std::string(magic.begin(), magic.end()) != "ESHTAC01") file.fail("fixture version");
  auto const count{file.integer<uint32_t>()};
  std::array constexpr modes{behavior::attacking, behavior::flanking, behavior::facing_off, behavior::attacking_base, behavior::attacking_flyer};
  for(uint32_t row{0}; row != count; ++row) {
    auto const type{modes.at(file.integer<uint8_t>())};
    herc_attack_inputs input;
    input.actor.position = {file.integer<int32_t>(), file.integer<int32_t>(), file.integer<int32_t>()};
    input.target.position = {file.integer<int32_t>(), file.integer<int32_t>(), file.integer<int32_t>()};
    input.actor.heading = file.integer<uint16_t>(); input.target.heading = file.integer<uint16_t>();
    input.actor.view_offset = file.integer<int16_t>(); input.target.view_offset = file.integer<int16_t>();
    auto state{read_state(file)}; auto const ratio{file.integer<int16_t>()};
    input.received_damage = file.integer<int32_t>(); auto command{file.integer<int16_t>()};
    input.actor_incapacitated = file.integer<uint8_t>() != 0; input.target_incapacitated = file.integer<uint8_t>() != 0;
    input.target_completed = file.integer<uint8_t>() != 0; input.base_can_retaliate = file.integer<uint8_t>() != 0;
    auto const expected_result{file.integer<uint8_t>() != 0};
    herc_target_geometry const expected_geometry{file.integer<int16_t>(), file.integer<int16_t>(), file.integer<int16_t>(), file.integer<int16_t>(), file.integer<int16_t>(), file.integer<int32_t>(), file.integer<int16_t>(), file.integer<int32_t>(), file.integer<int32_t>()};
    auto const expected_state{read_state(file)}; auto const expected_command{file.integer<int16_t>()};
    std::vector<std::array<int16_t, 4>> expected, actual;
    auto const events{file.integer<uint16_t>()};
    for(unsigned int i{0}; i != events; ++i) {
      auto const kind{file.integer<uint8_t>()};
      if(kind) expected.push_back({1, file.integer<int16_t>(), 0, 0});
      else expected.push_back({0, file.integer<int16_t>(), file.integer<int16_t>(), file.integer<int16_t>()});
    }
    auto const label{std::to_string(row) + " " + std::string{describe(type).name}};
    if(geometry_fields(relative_herc_target(input.actor, input.target)) != geometry_fields(expected_geometry)) file.fail(label + " relative geometry");
    auto const result{attack_with_herc(type, state, command, input, ratio, {
      .move{[&](movement_request const &control){ actual.push_back({0, control.flags, control.speed, control.turn}); }},
      .aim{[&](int16_t facing){ actual.push_back({1, facing, 0, 0}); }},
    })};
    if(actual != expected) {
      for(auto const &event : actual) std::cerr << "actual " << event[0] << ' ' << event[1] << ' ' << event[2] << ' ' << event[3] << '\n';
      for(auto const &event : expected) std::cerr << "expected " << event[0] << ' ' << event[1] << ' ' << event[2] << ' ' << event[3] << '\n';
      file.fail(label + " movement/aim call order");
    }
    if(result != expected_result || command != expected_command || state_fields(state) != state_fields(expected_state)) file.fail(label + " state/result");
  }
  file.require_end(); if(count != 3200) file.fail("tactics coverage");
  std::cout << count << " original HERC attack movement decisions agree\n";
  return EXIT_SUCCESS;
} catch(std::exception const &error) { std::cerr << error.what() << '\n'; return EXIT_FAILURE; }
