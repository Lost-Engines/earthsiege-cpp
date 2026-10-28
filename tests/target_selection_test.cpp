#include <cstdlib>
#include <iostream>
#include "resources/resource_store.h"
#include "simulation/target_selection.h"

namespace {

std::optional<size_t> optional(earthsiege::binary_reader &file) {
  auto const value{file.integer<int16_t>()};
  return value < 0 ? std::nullopt : std::optional<size_t>{static_cast<size_t>(value)};
}
earthsiege::random_state::state random(earthsiege::binary_reader &file) {
  earthsiege::random_state::state result;
  for(auto &v : result.words) v = file.integer<uint16_t>();
  result.first = file.integer<uint8_t>(); result.second = file.integer<uint8_t>();
  return result;
}

} // anonymous namespace

int main(int const argc, char **argv) try {
  if(argc != 2) return EXIT_FAILURE;
  auto const data{earthsiege::read_file(argv[1])}; earthsiege::binary_reader file{data};
  auto const magic{file.take(8)};
  if(std::string(magic.begin(), magic.end()) != "ESTARG02") file.fail("fixture version");
  auto const count{file.integer<uint32_t>()};
  for(size_t row{0}; row != count; ++row) {
    auto const flags{static_cast<earthsiege::target_selection_flags>(file.integer<uint8_t>())};
    auto const mission{file.integer<int16_t>()};
    auto const hud{optional(file)}, expected{optional(file)};
    earthsiege::target_objective group;
    group.order = file.integer<int16_t>(); group.leader = file.integer<uint16_t>(); group.representative = optional(file);
    std::vector<size_t> targets(file.integer<uint16_t>());
    for(auto &v : targets) v = file.integer<uint16_t>();
    group.actors = targets;
    earthsiege::random_state rng{random(file)};
    auto const expected_random{random(file)};
    auto const actor_count{file.integer<uint16_t>()};
    std::vector<std::vector<uint8_t>> knowledge(actor_count);
    std::vector<earthsiege::target_candidate> actors(actor_count);
    for(size_t index{0}; index != actor_count; ++index) {
      auto &a{actors[index]};
      for(unsigned int axis{0}; axis != 3; ++axis) a.position[axis] = file.integer<int32_t>();
      a.heading = file.integer<uint16_t>(); a.kind = static_cast<earthsiege::combat_actor_kind>(file.integer<int16_t>());
      a.side = file.integer<uint8_t>(); a.strength = file.integer<int16_t>();
      auto const known{file.take(actor_count)}; knowledge[index].assign(known.begin(), known.end()); a.known = knowledge[index];
      a.seen = file.integer<uint8_t>(); a.dead = file.integer<uint8_t>(); a.removed = file.integer<uint8_t>();
      a.disabled = file.integer<uint8_t>(); a.unavailable = file.integer<uint8_t>(); a.objective_event = file.integer<uint8_t>();
      a.player = file.integer<uint8_t>(); a.ignore_hud_target = file.integer<uint8_t>();
      bool const controller{file.integer<uint8_t>() != 0}, active{file.integer<uint8_t>() != 0};
      if(controller) a.controller_attacking = active;
      a.target = optional(file); a.attackers = file.integer<int16_t>(); a.compares_combat_strength = file.integer<uint8_t>() != 0;
    }
    auto const selected{earthsiege::select_target(actors, 0, flags, group, mission, hud, rng)};
    if(selected != expected) file.fail("target selection " + std::to_string(row));
    auto const &state{rng.snapshot()};
    if(state.words != expected_random.words || state.first != expected_random.first || state.second != expected_random.second) file.fail("target selection RNG " + std::to_string(row));
  }
  file.require_end();
  if(count != 4500) file.fail("target selection coverage differs");
  std::cout << count << " original target selections and complete random states agree\n";
  return EXIT_SUCCESS;
} catch(std::exception const &error) {
  std::cerr << error.what() << '\n'; return EXIT_FAILURE;
}
