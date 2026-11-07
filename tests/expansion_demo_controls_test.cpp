#include <iostream>
#include <stdexcept>
#include "resources/resource_store.h"
#include "resources/mission_script.h"
#include "simulation/aim_control.h"
#include "shell/tactical_controller.h"
#include "simulation/component_damage.h"
#include "simulation/navigation.h"
#include "simulation/shield_power.h"
#include "simulation/weapon_controls.h"

int main(int argc, char **argv) try {
  if(argc != 2) return 1;
  auto const bytes{earthsiege::read_file(argv[1])};
  earthsiege::binary_reader file{bytes};
  auto const magic{file.take(8)};
  if(std::string(magic.begin(), magic.end()) != "ESXCTRL1") file.fail("fixture version");
  auto const word{[&]{return file.integer<int16_t>();}};
  auto const aims{file.integer<uint32_t>()};
  for(uint32_t row{}; row < aims; ++row) {
    earthsiege::aiming_profile profile;
    profile.rate = word(); profile.acceleration = word(); profile.minimum = word(); profile.maximum = word();
    auto const ratio{word()}, input{word()};
    earthsiege::aiming_state state{word(), word()}, expected{word(), word()};
    bool const tracking{file.integer<uint8_t>() != 0}; auto const target{word()};
    earthsiege::update_aim(state, profile, input, ratio, tracking ? std::optional<int16_t>{target} : std::nullopt);
    if(state.rate != expected.rate || state.angle != expected.angle) file.fail("expansion aim " + std::to_string(row));
  }
  auto const shields{file.integer<uint32_t>()};
  for(uint32_t row{}; row < shields; ++row) {
    earthsiege::shield_power state{word(), word(), word(), word()};
    auto const budget{word()}, front{word()}, rear{word()}, expected{word()};
    auto const remaining{earthsiege::recharge_shields(state, budget, 5)};
    if(remaining != expected || state.front != front || state.rear != rear) file.fail("expansion shield " + std::to_string(row));
  }
  auto const avoidance{file.integer<uint32_t>()};
  for(uint32_t row{}; row < avoidance; ++row) {
    earthsiege::avoidance_state state;
    state.position = {file.integer<int32_t>(), file.integer<int32_t>(), file.integer<int32_t>()};
    state.heading = file.integer<uint16_t>(); state.current_speed = word(); state.collision_countdown = file.integer<int32_t>();
    state.forward_collision = file.integer<uint8_t>() != 0; state.collision_bias = word();
    earthsiege::movement_request request{word(), word(), word()};
    auto const count{file.integer<uint16_t>()}; std::vector<vec2<int32_t>> line;
    for(unsigned i{}; i < count; ++i) line.push_back({file.integer<int32_t>(), file.integer<int32_t>()});
    auto const speed{word()}, turn{word()};
    earthsiege::avoid_obstacles(request, state, {}, [](auto, auto const &, auto const &){return std::nullopt;},
      {.expansion{true}, .friendly_fire_line{line}, .reverse_rate{-140}});
    if(request.speed != speed || request.turn != turn) file.fail("expansion avoidance " + std::to_string(row) + ": " + std::to_string(request.speed) + "/" + std::to_string(speed) + " " + std::to_string(request.turn) + "/" + std::to_string(turn));
  }
  auto const accumulators{file.integer<uint32_t>()};
  for(uint32_t row{}; row < accumulators; ++row) {
    auto amount{word()}; auto const capacity{word()}; auto current{word()};
    auto const expected_amount{word()}, expected_current{word()}, expected_dead{word()};
    auto const dead{earthsiege::accumulate_damage(amount, capacity, current, earthsiege::game_edition::expansion)};
    if(amount != expected_amount || current != expected_current || dead != (expected_dead != 0)) file.fail("expansion damage accumulator " + std::to_string(row));
  }
  auto const capacitors{file.integer<uint32_t>()};
  for(uint32_t row{}; row < capacitors; ++row) {
    earthsiege::capacitor state; state.setting = word(); state.charge = word(); state.fast = word() != 0;
    auto const inhibited{word()}, budget{word()}, charge{word()}, fast{word()}, remaining{word()};
    auto const actual{earthsiege::charge_capacitor(state, inhibited != 0, budget, 256, true)};
    if(actual != remaining || state.charge != charge || state.fast != (fast != 0)) file.fail("expansion EMP recharge " + std::to_string(row));
  }
  auto const pans{file.integer<uint32_t>()};
  for(uint32_t row{}; row < pans; ++row) {
    auto const maximum{file.integer<int32_t>()}, height{file.integer<int32_t>()}, expected{file.integer<int32_t>()};
    earthsiege::tactical_controller controller{true};
    controller.initialize_height(maximum); controller.camera({0, 0, height}); controller.pan(1, 0);
    if(controller.state().pan.x != expected) file.fail("expansion map pan " + std::to_string(row));
  }
  auto const definitions{file.integer<uint32_t>()};
  for(uint32_t row{}; row < definitions; ++row)
    if(earthsiege::describe(static_cast<earthsiege::behavior>(row), true).properties != file.integer<uint16_t>()) file.fail("expansion controller flags " + std::to_string(row));
  auto const invulnerability{file.integer<uint32_t>()};
  for(uint32_t row{}; row < invulnerability; ++row) {
    earthsiege::mission_header mission;
    mission.campaign_mode = word(); mission.invulnerable_player = word(); mission.difficulty = word();
    if(mission.player_is_invulnerable(earthsiege::game_edition::expansion) != (word() != 0)) file.fail("expansion invulnerability " + std::to_string(row));
  }
  file.require_end();
  std::cout << aims << " tracking controls, " << shields << " shield recharges, " << avoidance << " firing-line avoidance cases, " << accumulators << " damage accumulators, " << capacitors << " EMP recharges and " << pans << " map pans agree with original instructions\n";
  return 0;
} catch(std::exception const &error) {std::cerr << error.what() << '\n'; return 1;}
