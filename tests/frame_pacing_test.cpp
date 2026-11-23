#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include "resources/binary_reader.h"
#include "simulation/frame_clock.h"
using earthsiege::binary_reader;
using earthsiege::frame_clock;
using earthsiege::frame_clock_state;
using earthsiege::read_file;
namespace {
void check_world_clock() {
  // Sample the same ten seconds at several display refresh rates, including
  // polls that share a timer tick and polls that overshoot the next deadline.
  for(unsigned hz : {30u, 60u, 75u, 144u, 240u}) {
    frame_clock clock;
    unsigned ticks{0};
    unsigned steps{0};
    for(unsigned poll{1}; poll <= hz * 10; ++poll) {
      auto const now{poll * 120 / hz};
      clock.advance(now - ticks);
      ticks = now;
      if(auto ratio{clock.take_world()}) {
        if(*ratio != 256) throw std::runtime_error{"world update changed speed with display refresh"};
        ++steps;
      }
      if(steps != ticks / 12) throw std::runtime_error{"world update missed its deadline"};
    }
  }
  frame_clock clock;
  clock.advance(120);
  if(clock.take_world() != 450 || clock.take_world()) throw std::runtime_error{"world stall clamp"};
  clock.advance(11);
  if(clock.take_world()) throw std::runtime_error{"stall left a queued world update"};
  clock.advance(1);
  if(clock.take_world() != 256 || clock.state().slow != 66) throw std::runtime_error{"ordinary pace after stall"};
}
}
int main(int argc, char **argv) try {
  if(argc != 2) return EXIT_FAILURE;
  auto data{read_file(argv[1])};
  binary_reader f{data};
  auto magic{f.take(8)};
  if(std::string(magic.begin(), magic.end()) != "ESPACE01") f.fail("fixture version");
  auto traces{f.integer<uint32_t>()};
  auto steps{f.integer<uint32_t>()};
  for(unsigned t{0}; t < traces; ++t) {
    frame_clock_state initial;
    initial.maximum = f.integer<int16_t>();
    initial.nominal = f.integer<int16_t>();
    initial.alternate_nominal = f.integer<int16_t>();
    initial.fast = f.integer<uint32_t>();
    initial.slow = f.integer<uint32_t>();
    initial.countdown = initial.maximum;
    initial.elapsed = initial.nominal;
    frame_clock clock{initial};
    for(unsigned s{0}; s < steps; ++s) {
      clock.advance(f.integer<uint16_t>());
      auto expected{f.integer<uint8_t>() != 0};
      if(clock.ready() != expected || clock.take().has_value() != expected) f.fail("pacing gate");
      auto const &v{clock.state()};
      for(auto word : {v.countdown, v.elapsed, v.ratio, v.alternate_ratio}) {
        if(word != f.integer<int16_t>()) f.fail("frame ratio/counter");
      }
      for(auto word : {v.fast, v.slow}) {
        if(word != f.integer<uint32_t>()) f.fail("clock wrap");
      }
    }
  }
  f.require_end();
  check_world_clock();
  std::cout << traces * steps << " original timer/pacing states agree\n";
  return EXIT_SUCCESS;
} catch(std::exception const &e) {
  std::cerr << e.what() << '\n';
  return EXIT_FAILURE;
}
