#include <array>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include "resources/binary_reader.h"
#include "simulation/hit_dispatch.h"
#include "simulation/model_math.h"

namespace {

earthsiege::random_state seeded() {
  earthsiege::random_state::state state;
  for(size_t i{0}; i != state.words.size(); ++i) state.words[i] = static_cast<uint16_t>((i * 7919 + 12345) & 32767);
  state.first = 31; state.second = 0;
  return earthsiege::random_state{state};
}

} // namespace

int main(int const argc, char **argv) try {
  if(argc != 2) return EXIT_FAILURE;
  auto const bytes{earthsiege::read_file(argv[1])};
  earthsiege::binary_reader file{bytes};
  auto const magic{file.take(8)};
  if(std::string(magic.begin(), magic.end()) != "ESHITD01") file.fail("fixture version");
  auto const count{file.integer<uint32_t>()};
  auto const ground_count{file.integer<uint32_t>()};
  for(unsigned int row{0}; row != count; ++row) {
    earthsiege::impact_request request;
    request.transform.translation = {0, 0, 10000};
    request.reach = file.integer<int32_t>(); request.radius = 50; request.damage.armour = 100; request.damage.shield = 200;
    auto const expected_result{file.integer<int32_t>()};
    std::array<earthsiege::impact_actor, 7> actors;
    actors[0].position = {0, 0, 10000}; actors[6].position = {0, 10000, 10000};
    std::array<int32_t, 5> depths{};
    std::vector<std::vector<int32_t>> events;
    for(size_t i{0}; i != depths.size(); ++i) {
      auto &actor{actors[i + 1]};
      actor.side = file.integer<uint8_t>();
      actor.group = file.integer<uint8_t>() ? 0 : i + 1;
      actor.waiting = file.integer<uint8_t>() != 0;
      actor.dead = file.integer<uint8_t>() != 0;
      actor.disabled = file.integer<uint8_t>() != 0;
      actor.inactive = file.integer<uint8_t>() != 0;
      actor.quiet = file.integer<uint8_t>() != 0;
      file.integer<uint8_t>();                                                // native trigger operation; activation itself has separate references
      depths[i] = file.integer<int32_t>();
      actor.trigger = i;
      actor.hit = [&, i](earthsiege::impact_request const &ray) -> std::optional<int32_t> {
        auto const depth{depths[i]};
        bool const hit{depth > 0 && depth < ray.reach};
        events.push_back({0, static_cast<int32_t>(i), ray.reach, hit ? depth : 0});
        return hit ? std::optional<int32_t>{depth} : std::nullopt;
      };
    }
    std::vector<std::vector<int32_t>> expected_events;
    auto const event_count{file.integer<uint16_t>()};
    for(unsigned int event{0}; event != event_count; ++event) {
      auto const kind{file.integer<uint8_t>()};
      std::vector<int32_t> entry{kind};
      for(unsigned int field{0}; field != (kind == 0 ? 3U : kind == 3 ? 0U : 1U); ++field) entry.push_back(file.integer<int32_t>());
      expected_events.push_back(std::move(entry));
    }
    auto random{seeded()};
    earthsiege::impact_world world{actors, {}, random, {}};
    world.events.trigger = [&](size_t const index) { events.push_back({1, static_cast<int32_t>(index)}); };
    world.events.react = [&](size_t const victim, size_t const owner, int16_t const damage) {
      if(owner != 0 || damage != 100) file.fail("reaction arguments");
      events.push_back({2, static_cast<int32_t>(victim - 1)});
    };
    world.events.friendly_warning = [&](size_t const companion) { if(companion != 6) file.fail("friendly warning companion"); events.push_back({3}); };
    std::array<size_t, 2> const members{0, 6};
    auto const result{earthsiege::dispatch_impact(request, {0, row % 3 == 0, members, std::nullopt}, world)};
    if((result ? *result + 1 : 0) != expected_result || events != expected_events) file.fail("dispatch reference " + std::to_string(row) + " differs");
    bool enemy_hit{false};
    for(auto const &event : expected_events) if(event[0] == 0 && event[3] != 0 && actors[static_cast<size_t>(event[1]) + 1].side != 0) enemy_hit = true;
    if(actors[0].hit_enemy != enemy_hit) file.fail("owner enemy-hit latch");
  }
  for(unsigned int row{0}; row != ground_count; ++row) {
    auto const pitch{file.integer<uint16_t>()}; auto const yaw{file.integer<uint16_t>()};
    auto const height{file.integer<int32_t>()}; auto const reach{file.integer<int32_t>()};
    auto const expected{file.integer<int32_t>()}; auto const effects{file.integer<uint8_t>()};
    earthsiege::impact_request request;
    request.transform = earthsiege::rotation_transform({pitch, 0, yaw}); request.transform.translation = {0, 0, height}; request.reach = reach; request.radius = 50;
    request.damage.ground_effects = {104, 105, 106, 107};
    std::array<earthsiege::impact_actor, 1> actors;
    auto random{seeded()};
    auto expected_random{random};
    earthsiege::impact_world world{actors, {}, random, {}};
    unsigned int effect_count{0};
    world.events.effect = [&](vec3<int32_t> const &position, int16_t const effect) {
      ++effect_count;
      vec3<int32_t> const expected_position{file.integer<int32_t>(), file.integer<int32_t>(), file.integer<int32_t>()};
      auto const expected_effect{file.integer<int16_t>()};
      if(position != expected_position || effect != expected_effect) file.fail("ground impact visual");
    };
    auto const result{earthsiege::dispatch_impact(request, {}, world)};
    if((result ? *result + 1 : 0) != expected || effect_count != effects) file.fail("ground reference " + std::to_string(row) + " differs");
    if(effects) expected_random.bounded(4);
    auto const &a{random.snapshot()}; auto const &b{expected_random.snapshot()};
    if(a.words != b.words || a.first != b.first || a.second != b.second) file.fail("ground effect random state");
  }
  file.require_end();
  if(count != 1200 || ground_count != 200) file.fail("dispatch reference coverage differs");
  std::cout << count << " original actor dispatches and " << ground_count << " ground impacts agree" << std::endl;
  return EXIT_SUCCESS;
} catch(std::exception const &error) {
  std::cerr << error.what() << std::endl;
  return EXIT_FAILURE;
}
