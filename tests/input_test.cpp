#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include "resources/binary_reader.h"
#include "simulation/frame_time.h"
#include "simulation/keyboard_input.h"

int main(int const argc, char **argv) try {
  if(argc != 2) return EXIT_FAILURE;
  auto const bytes{earthsiege::read_file(argv[1])};
  earthsiege::binary_reader file{bytes};
  auto const magic{file.take(8)};
  if(std::string(magic.begin(), magic.end()) != "ESINPUT1") file.fail("fixture version");
  auto const read_state{[&]{
    earthsiege::keyboard_latches result;
    for(auto *group : {&result.held, &result.released, &result.consumed}) for(auto &value : *group) value = file.integer<uint8_t>();
    return result;
  }};
  auto const check_state{[&](earthsiege::keyboard_input const &input){
    auto const expected{read_state()};
    auto const &actual{input.state()};
    if(actual.held != expected.held || actual.released != expected.released || actual.consumed != expected.consumed) file.fail("keyboard latch state differs");
  }};
  auto const check_sample{[&](earthsiege::keyboard_sample const &actual){
    for(auto const value : {actual.movement.x, actual.movement.y, actual.aiming.x, actual.aiming.y}) if(value != file.integer<int32_t>()) file.fail("keyboard axis differs");
    for(auto const value : {actual.fire, actual.enter}) if(value != (file.integer<uint8_t>() != 0)) file.fail("keyboard button differs");
  }};
  auto const combinations{file.integer<uint32_t>()};
  for(unsigned int index{0}; index != combinations; ++index) {
    auto initial{read_state()};
    bool const alternate{file.integer<uint8_t>() != 0};
    initial.alt = file.integer<uint8_t>() != 0;
    earthsiege::keyboard_input input{initial};
    check_sample(input.sample(alternate));
    check_state(input);
  }
  auto const events{file.integer<uint32_t>()};
  earthsiege::keyboard_input input;
  for(unsigned int index{0}; index != events; ++index) {
    auto const key{static_cast<earthsiege::held_key>(file.integer<uint8_t>())};
    bool const released{file.integer<uint8_t>() != 0};
    bool const alt{file.integer<uint8_t>() != 0};
    input.event(key, !released, alt);
    check_state(input);
    if(input.state().alt != alt) file.fail("held-key modifier differs");
    if(file.integer<uint8_t>()) {
      bool const alternate{file.integer<uint8_t>() != 0};
      check_sample(input.sample(alternate));
      check_state(input);
    }
  }
  input.clear();
  auto const cleared{input.sample()};
  if(cleared.movement.x || cleared.movement.y || cleared.aiming.x || cleared.aiming.y || cleared.fire || cleared.enter) file.fail("focus-loss reset retains input");
  auto const periods{file.integer<uint16_t>()};
  for(unsigned int index{0}; index != periods; ++index) {
    auto const elapsed{file.integer<int16_t>()};
    auto const nominal{file.integer<int16_t>()};
    if(earthsiege::ratio_from_period(elapsed, nominal) != file.integer<int16_t>()) file.fail("frame ratio differs");
  }
  auto const times{file.integer<uint16_t>()};
  for(unsigned int index{0}; index != times; ++index) {
    auto const value{file.integer<int16_t>()};
    auto const ratio{file.integer<int16_t>()};
    if(earthsiege::scale_frame_time(value, ratio) != file.integer<int16_t>()) file.fail("scaled time differs");
  }
  file.require_end();
  if(combinations != 4096 || events != 3000 || periods != 71 || times != 290) file.fail("input coverage differs");
  std::cout << combinations << " original keyboard combinations, " << events << " events and " << periods + times << " time cases agree" << std::endl;
  return EXIT_SUCCESS;
} catch(std::exception const &error) {
  std::cerr << error.what() << std::endl;
  return EXIT_FAILURE;
}
