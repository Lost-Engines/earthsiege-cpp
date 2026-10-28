#include <cstdlib>
#include <iostream>
#include "resources/binary_reader.h"
#include "simulation/herc_sensors.h"
using namespace earthsiege;
int main(int argc, char **argv) try {
  if(argc != 2) return EXIT_FAILURE;
  auto const bytes{read_file(argv[1])}; binary_reader file{bytes}; auto const magic{file.take(8)};
  if(std::string(magic.begin(), magic.end()) != "ESHSN001") file.fail("fixture version");
  auto const number{[&]{return file.integer<int32_t>();}};
  auto const boolean{[&]{return number() != 0;}};
  auto const short_value{[&]{return static_cast<int16_t>(number());}};
  auto const read_random{[&]{random_state::state rng; for(auto &v : rng.words) v = static_cast<uint16_t>(number()); rng.first = static_cast<uint8_t>(number()); rng.second = static_cast<uint8_t>(number()); return rng;}};
  auto const count{number()};
  for(int trial{0}; trial != count; ++trial) {
    herc_sensor_inputs input; herc_sensor_state state; herc_impact_state impact;
    bool const selected{boolean()}; input.player = boolean(); auto const channel{number()};
    if(channel < 5) input.selected_channel = static_cast<size_t>(channel);
    for(auto &v : input.missile_counts) v = short_value();
    input.ecm_active = boolean(); input.ecm_pulse = boolean(); auto const ratio{short_value()};
    input.position = {number(), number(), number()};
    herc_sensor_target target; target.position = {number(), number(), number()};
    input.heading = static_cast<uint16_t>(number()); input.torso = short_value(); target.herc = number() == 0;
    for(auto &v : state.acquisition_remaining) v = short_value();
    state.interference_remaining = short_value(); impact.ecm_remaining = short_value(); state.damage_window_remaining = short_value(); impact.damage_received = number();
    bool changed{boolean()}; state.interfered = boolean(); impact.detectable = boolean(); target.ecm_active = boolean(); target.detectable = boolean(); bool alert{boolean()};
    if(selected) input.target = target;
    random_state random{read_random()};
    herc_sensor_state expected; for(auto &v : expected.acquisition_remaining) v = short_value();
    expected.interference_remaining = short_value(); auto const ecm_timer{short_value()}; expected.damage_window_remaining = short_value(); auto const damage{number()};
    expected.hud_locked = boolean(); expected.interfered = boolean(); bool const expected_changed{boolean()}; expected.ecm_active = boolean(); bool const expected_alert{boolean()};
    for(auto &v : expected.locks) v = boolean();
    auto const rng{read_random()}; auto const hud{number()}; int hud_calls{0};
    advance_herc_sensors(state, impact, changed, input, ratio, random, [&]{alert = true;}, [&]{++hud_calls;});
    auto const label{std::to_string(trial)};
    if(state.acquisition_remaining != expected.acquisition_remaining || state.interference_remaining != expected.interference_remaining || state.damage_window_remaining != expected.damage_window_remaining || state.hud_locked != expected.hud_locked || state.interfered != expected.interfered || state.ecm_active != expected.ecm_active || state.locks != expected.locks || impact.ecm_remaining != ecm_timer || impact.damage_received != damage || changed != expected_changed || alert != expected_alert || hud_calls != hud) file.fail(label + " sensor state");
    auto const &actual{random.snapshot()}; if(actual.words != rng.words || actual.first != rng.first || actual.second != rng.second) file.fail(label + " sensor RNG");
  }
  file.require_end(); if(count != 4096) file.fail("sensor coverage");
  std::cout << count << " original ECM/sensor/lock frames agree\n";
  return EXIT_SUCCESS;
} catch(std::exception const &error) {std::cerr << error.what() << '\n'; return EXIT_FAILURE;}
