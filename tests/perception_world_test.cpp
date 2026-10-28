#include <array>
#include <cstdlib>
#include <iostream>
#include <string>
#include "resources/binary_reader.h"
#include "simulation/perception.h"

namespace {
using namespace earthsiege;
std::vector<actor_awareness> read_state(binary_reader &file) {
  std::vector<actor_awareness> result;
  for(unsigned int i{0}; i != 8; ++i) {
    result.emplace_back(8); auto &s{result.back()};
    for(auto &v : s.known) v = file.integer<uint8_t>();
    for(auto &v : s.visible) v = file.integer<uint8_t>();
    s.visibility_remaining = file.integer<int16_t>(); s.refresh_remaining = file.integer<int16_t>();
    s.seen = file.integer<uint8_t>() != 0; s.alerted = file.integer<uint8_t>() != 0; s.event_busy = file.integer<uint8_t>() != 0;
  }
  return result;
}
random_state::state read_random(binary_reader &file) {
  random_state::state result; for(auto &v : result.words) v = file.integer<uint16_t>(); result.first = file.integer<uint8_t>(); result.second = file.integer<uint8_t>(); return result;
}
struct input {
  vec3<int32_t> position;
  uint16_t heading;
  int16_t offset;
  bool omni;
  uint8_t side;
  bool waiting,dead,disabled,player,detectable,trigger;
  int16_t counter,operation;
  bool latched;
};
}
int main(int const argc, char **argv) try {
  if(argc != 2) return EXIT_FAILURE;
  auto const bytes{read_file(argv[1])}; binary_reader file{bytes}; auto const magic{file.take(8)};
  if(std::string(magic.begin(),magic.end()) != "ESPSCAN1") file.fail("fixture version");
  unsigned int passes{0}, boundary_calls{0}; auto const count{file.integer<uint16_t>()};
  for(unsigned int row{0}; row != count; ++row) {
    std::array<input,8> inputs;
    for(auto &a : inputs) {
      a.position = vec3<int32_t>{file.integer<int32_t>(),file.integer<int32_t>(),file.integer<int32_t>()};
      a.heading = file.integer<uint16_t>(); a.offset = file.integer<int16_t>(); a.omni = file.integer<uint8_t>() != 0; a.side = file.integer<uint8_t>();
      a.waiting = file.integer<uint8_t>() != 0; a.dead = file.integer<uint8_t>() != 0; a.disabled = file.integer<uint8_t>() != 0; a.player = file.integer<uint8_t>() != 0;
      a.detectable = file.integer<uint8_t>() != 0; a.trigger = file.integer<uint8_t>() != 0;
      a.counter = file.integer<int16_t>(); a.operation = file.integer<int16_t>(); a.latched = file.integer<uint8_t>() != 0;
    }
    auto states{read_state(file)}; random_state random{read_random(file)}; auto const range{file.integer<int32_t>()};
    std::array<std::array<bool,8>,8> visibility; for(auto &line : visibility) for(auto &v : line) v = file.integer<uint8_t>() != 0;
    mission_script script; std::vector<perception_actor> actors; std::array<int16_t,8> counters{};
    for(size_t i{0}; i != inputs.size(); ++i) {
      auto const &a{inputs[i]}; mission_trigger trigger; trigger.areas.fill(-1); trigger.counter = static_cast<int16_t>(i); trigger.counter_operation = a.operation; script.triggers.push_back(trigger);
      actors.push_back({vec3<int32_t>{a.position},a.side,a.waiting,a.trigger ? std::optional<size_t>{i} : std::nullopt,states[i],a.dead,a.disabled,a.player,a.detectable});
    }
    mission_triggers triggers{script};
    for(size_t i{0}; i != inputs.size(); ++i) { if(inputs[i].latched) triggers.activate(i,counters); counters[i] = inputs[i].counter; }
    std::vector<std::array<int32_t,3>> actual;
    perception_callbacks callbacks;
    callbacks.line_of_sight = [&](size_t from,size_t to){ actual.push_back({0,static_cast<int32_t>(from),static_cast<int32_t>(to)}); return visibility[from][to]; };
    callbacks.orientation_offset = [&](size_t actor){ actual.push_back({1,static_cast<int32_t>(actor),inputs[actor].offset}); return inputs[actor].offset; };
    callbacks.within_view = [&](size_t actor,int16_t angle){ actual.push_back({2,static_cast<int32_t>(actor),angle}); return inputs[actor].omni || within_actor_view(inputs[actor].heading,angle); };
    callbacks.noticed = [&](size_t from,size_t to){ actual.push_back({3,static_cast<int32_t>(from),static_cast<int32_t>(to)}); };
    auto const ticks{file.integer<uint16_t>()};
    for(unsigned int tick{0}; tick != ticks; ++tick) {
      auto const ratio{file.integer<int16_t>()}; auto const expected{read_state(file)}; auto const expected_random{read_random(file)};
      std::array<bool,8> latched; for(auto &v : latched) v = file.integer<uint8_t>() != 0;
      std::array<int16_t,8> expected_counters; for(auto &v : expected_counters) v = file.integer<int16_t>();
      std::vector<std::array<int32_t,3>> events(file.integer<uint16_t>()); for(auto &event : events) for(auto &v : event) v = file.integer<int32_t>();
      actual.clear(); advance_perception(actors,ratio,range,random,triggers,counters,callbacks);
      auto const label{std::to_string(row) + '/' + std::to_string(tick)};
      for(size_t i{0}; i != states.size(); ++i) {
        auto const &a{states[i]}; auto const &e{expected[i]};
        if(a.known != e.known || a.visible != e.visible || a.visibility_remaining != e.visibility_remaining || a.refresh_remaining != e.refresh_remaining || a.seen != e.seen || a.alerted != e.alerted || a.event_busy != e.event_busy) file.fail(label + " actor " + std::to_string(i) + " awareness state");
        if(triggers.latched(i) != latched[i]) file.fail(label + " trigger latch");
      }
      if(counters != expected_counters) file.fail(label + " counters");
      if(actual != events) file.fail(label + " callback order");
      auto const &rng{random.snapshot()}; if(rng.words != expected_random.words || rng.first != expected_random.first || rng.second != expected_random.second) file.fail(label + " RNG");
      ++passes; boundary_calls += static_cast<unsigned int>(actual.size());
    }
  }
  file.require_end(); if(count != 384 || passes != 2304 || boundary_calls < 1000) file.fail("perception coverage");
  std::cout << passes << " original world perception passes and " << boundary_calls << " ordered callbacks agree\n"; return EXIT_SUCCESS;
} catch(std::exception const &error) { std::cerr << error.what() << '\n'; return EXIT_FAILURE; }
