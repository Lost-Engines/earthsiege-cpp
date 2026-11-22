#include <cstdlib>
#include <iostream>
#include "shell/mission_preferences.h"
using earthsiege::binary_reader;
using earthsiege::mission_preference_action;
using earthsiege::mission_preferences;
using earthsiege::preferences;
using earthsiege::read_file;
int main(int argc, char **argv) try {
  if(argc != 2) return EXIT_FAILURE;
  auto data{read_file(argv[1])};
  binary_reader f{data};
  auto magic{f.take(8)};
  if(std::string(magic.begin(), magic.end()) != "ESPRFP01") f.fail("fixture version");
  auto count{f.integer<uint32_t>()};
  unsigned total{0};
  for(unsigned i{0}; i < count; ++i) {
    preferences p{f.take(17)};
    bool radio{f.integer<uint8_t>() != 0};
    auto actions{f.integer<uint16_t>()};
    mission_preferences state{p, radio};
    auto check{[&]{
      for(auto const *values : {&p.current(), &p.saved(), &p.previous(), &state.persisted()}) {
        for(auto v : *values) {
          if(f.integer<uint8_t>() != v) f.fail("preference transaction mismatch");
        }
      }
    }};
    check();
    for(unsigned j{0}; j < actions; ++j) {
      auto index{f.integer<uint8_t>()};
      bool reverse{f.integer<uint8_t>() != 0};
      auto result{state.activate(index, reverse)};
      if((result == mission_preference_action::done) != (index == 8)) f.fail("Done action");
      check();
      ++total;
    }
  }
  f.require_end();
  std::cout << total << " original in-flight preference changes agree\n";
  return EXIT_SUCCESS;
} catch(std::exception const &e) {
  std::cerr << e.what() << '\n';
  return EXIT_FAILURE;
}
