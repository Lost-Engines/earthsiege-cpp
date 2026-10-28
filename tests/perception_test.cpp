#include <cstdlib>
#include <iostream>
#include "resources/resource_store.h"
#include "simulation/perception.h"
#include "simulation/target_selection.h"

int main(int const argc, char **argv) try {
  if(argc != 2) return EXIT_FAILURE;
  auto const data{earthsiege::read_file(argv[1])}; earthsiege::binary_reader file{data};
  auto const magic{file.take(8)};
  if(std::string(magic.begin(), magic.end()) != "ESPER001") file.fail("fixture version");
  auto const count{file.integer<uint32_t>()};
  for(size_t row{0}; row != count; ++row) {
    auto const require{[&](bool const result, char const *field){ if(!result) file.fail("perception " + std::to_string(row) + ": " + field); }};
    auto const kind{file.integer<uint8_t>()};
    if(kind == 0) {
      std::array<uint8_t, 1> known{file.integer<uint8_t>()};
      earthsiege::target_candidate observer; observer.known = known;
      earthsiege::target_candidate target;
      target.seen = file.integer<uint8_t>(); target.position.x = file.integer<int32_t>();
      require(earthsiege::recognizes(observer, target, 0) == (file.integer<uint8_t>() != 0), "recognition");
    } else if(kind == 1) {
      auto const actors{file.integer<uint8_t>()}; auto const source{file.integer<uint8_t>()}, target{file.integer<uint8_t>()};
      earthsiege::mission_script script;
      std::vector<earthsiege::actor_awareness> awareness;
      awareness.reserve(actors);
      std::vector<earthsiege::perception_actor> entries;
      std::vector<int16_t> counters, expected_counters;
      std::vector<uint8_t> expected_known, expected_latches, initial_latches;
      for(size_t index{0}; index != actors; ++index) {
        awareness.emplace_back(actors);
        auto const side{file.integer<uint8_t>()}; bool const waiting{file.integer<uint8_t>() != 0};
        vec3<int32_t> const position{file.integer<int32_t>(), file.integer<int32_t>(), file.integer<int32_t>()};
        awareness.back().event_busy = file.integer<uint8_t>(); initial_latches.push_back(file.integer<uint8_t>());
        counters.push_back(file.integer<int16_t>());
        earthsiege::mission_trigger trigger;
        trigger.areas.fill(-1); trigger.counter = static_cast<int16_t>(index); trigger.counter_operation = file.integer<int16_t>();
        script.triggers.push_back(trigger);
        expected_known.push_back(file.integer<uint8_t>()); expected_latches.push_back(file.integer<uint8_t>()); expected_counters.push_back(file.integer<int16_t>());
        entries.push_back({vec3<int32_t>{position}, side, waiting, index, awareness.back()});
      }
      earthsiege::mission_triggers triggers{script};
      auto scratch{counters};
      for(size_t i{0}; i != actors; ++i) if(initial_latches[i]) triggers.activate(i, scratch);
      earthsiege::share_contact(entries, source, target, triggers, counters);
      require(awareness[target].alerted == (file.integer<uint8_t>() != 0), "target alert");
      for(size_t i{0}; i != actors; ++i) {
        require(awareness[i].known[target] == expected_known[i], "shared knowledge");
        require(triggers.latched(i) == (expected_latches[i] != 0), "trigger latch");
      }
      require(counters == expected_counters, "trigger counters");
    } else if(kind == 2) {
      earthsiege::actor_awareness observer{1}, target{1};
      target.visibility_remaining = file.integer<int16_t>(); observer.visibility_remaining = file.integer<int16_t>();
      observer.visible[0] = static_cast<uint8_t>(file.integer<int16_t>());
      auto const seed{file.integer<uint16_t>()};
      bool const expected{file.integer<int16_t>() != 0}; auto const clock{file.integer<int16_t>()};
      earthsiege::random_state::state initial;
      initial.words.fill(seed); initial.first = 0; initial.second = 31;
      earthsiege::random_state random{initial};
      unsigned int queries{0};
      auto const target_clock{target.visibility_remaining};
      require(earthsiege::cached_visibility(observer, target, 0, random, [&]{ ++queries; return true; }) == expected, "cached visibility");
      require(observer.visibility_remaining == clock && target.visibility_remaining == target_clock, "visibility clock owner");
      require(queries == static_cast<unsigned int>(target_clock == 0), "visibility query gating");
    } else file.fail("unknown perception record");
  }
  file.require_end();
  std::cout << count << " original recognition, contact sharing and visibility decisions agree\n";
  return EXIT_SUCCESS;
} catch(std::exception const &error) {
  std::cerr << error.what() << '\n'; return EXIT_FAILURE;
}
