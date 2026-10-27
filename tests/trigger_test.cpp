#include <array>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include "simulation/mission_triggers.h"

namespace {

vec3<int32_t> point(earthsiege::binary_reader &file) {
  return vec3<int32_t>{file.integer<int32_t>(), file.integer<int32_t>(), file.integer<int32_t>()};
}

earthsiege::mission_script area_script() {
  earthsiege::mission_script script;
  script.points.emplace_back(0, 0, 0);
  script.points.emplace_back(0, 0, 0);
  script.areas.push_back({.kind{1}, .first_point{0}, .second_point_or_radius{10}});
  earthsiege::mission_trigger trigger{.counter{7}, .counter_operation{6}};
  trigger.areas.fill(-1);
  trigger.areas[0] = 0;
  script.triggers.push_back(trigger);
  return script;
}

} // namespace

int main(int const argc, char **argv) try {
  if(argc != 2) return EXIT_FAILURE;
  auto const bytes{earthsiege::read_file(argv[1])};
  earthsiege::binary_reader file{bytes};
  auto const magic{file.take(8)};
  if(std::string(magic.begin(), magic.end()) != "ESTRIG01") file.fail("fixture version");
  auto const cases{file.integer<uint16_t>()};
  earthsiege::mission_triggers persistent{area_script()};
  std::array<int16_t, 100> persistent_counters{};
  for(unsigned int row{0}; row != cases; ++row) {
    auto const kind{file.integer<uint8_t>()};
    auto script{area_script()};
    std::array<int16_t, 100> counters{};
    bool matched{false};
    if(kind == 0) {
      auto const area_kind{file.integer<int16_t>()};
      script.points[0] = point(file);
      script.points[1] = point(file);
      auto const position{point(file)};
      auto const radius{file.integer<int32_t>()};
      bool const expected{file.integer<uint8_t>() != 0};
      script.areas[0] = {.kind{area_kind}, .first_point{0}, .second_point_or_radius{static_cast<int16_t>(area_kind ? radius / 10 : 1)}};
      earthsiege::mission_triggers triggers{script};
      matched = triggers.enter(0, position, counters) == expected && triggers.latched(0) == expected;
    } else if(kind == 1) {
      auto const counter{file.integer<int16_t>()};
      script.triggers[0].counter = counter;
      script.triggers[0].counter_operation = file.integer<int16_t>();
      auto const initial{file.integer<int16_t>()};
      auto const repeat{file.integer<uint16_t>()};
      auto const expected{file.integer<int16_t>()};
      if(counter >= 0) counters.at(static_cast<size_t>(counter)) = initial;
      earthsiege::mission_triggers triggers{script};
      for(unsigned int call{0}; call != repeat; ++call) triggers.activate(0, counters);
      matched = triggers.latched(0) && counters.at(counter >= 0 ? static_cast<size_t>(counter) : 0) == expected;
    } else if(kind == 2) {
      vec3<int32_t> const position{file.integer<int32_t>(), 0, 0};
      bool const expected{file.integer<uint8_t>() != 0};
      auto const expected_counter{file.integer<int16_t>()};
      matched = persistent.enter(0, position, persistent_counters) == expected && persistent_counters[7] == expected_counter;
    } else if(kind == 3) {
      script.triggers[0].mode = file.integer<uint8_t>();
      auto const inside{file.integer<uint8_t>()};
      auto const side{file.integer<uint8_t>()};
      auto const actor_kind{static_cast<earthsiege::actor_kind>(file.integer<uint8_t>())};
      bool const waiting{file.integer<uint8_t>() != 0};
      bool const expected{file.integer<uint8_t>() != 0};
      std::array<vec3<int32_t>, 3> positions;
      for(size_t i{0}; i != positions.size(); ++i) positions[i] = vec3<int32_t>{i == inside ? 0 : 500, 0, 0};
      std::array<earthsiege::trigger_group, 3> groups;
      for(size_t i{0}; i != groups.size(); ++i) groups[i] = {
        .kind{actor_kind}, .side{side}, .waiting{waiting},
        .positions{std::span<vec3<int32_t> const>{positions}.subspan(i, i == 0 ? 2 : 1)},
      };
      earthsiege::mission_triggers triggers{script};
      triggers.scan(positions[0], 0, groups, counters);
      matched = triggers.latched(0) == expected && counters[7] == static_cast<int16_t>(expected);
    } else file.fail("unknown trigger case");
    if(!matched) file.fail("original trigger case " + std::to_string(row) + " differs");
  }
  file.require_end();
  std::cout << cases << " original trigger boundaries, latches, counter actions and audiences agree" << std::endl;
  return EXIT_SUCCESS;
} catch(std::exception const &error) {
  std::cerr << error.what() << std::endl;
  return EXIT_FAILURE;
}
