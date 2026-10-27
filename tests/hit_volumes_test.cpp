#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include "resources/resource_store.h"
#include "simulation/hit_volumes.h"
#include "simulation/model_math.h"

namespace {

vec3<int32_t> point(earthsiege::binary_reader &file) {
  return {file.integer<int32_t>(), file.integer<int32_t>(), file.integer<int32_t>()};
}

earthsiege::model_transform transform(earthsiege::binary_reader &file) {
  auto result{earthsiege::rotation_transform({0, 0, file.integer<uint16_t>()})};
  result.translation = point(file);
  return result;
}

} // namespace

int main(int const argc, char **argv) try {
  if(argc != 3) return EXIT_FAILURE;
  earthsiege::resource_store resources{argv[1]};
  auto const bytes{earthsiege::read_file(argv[2])};
  earthsiege::binary_reader file{bytes};
  auto const magic{file.take(8)};
  if(std::string(magic.begin(), magic.end()) != "ESHIT001") file.fail("fixture version");
  auto const count{file.integer<uint32_t>()};
  std::map<std::string, std::vector<earthsiege::collision_part>> definitions;
  for(unsigned int row{0}; row != count; ++row) {
    auto const kind{file.integer<uint8_t>()};
    auto check{[&](bool const okay){if(!okay) file.fail("hit-volume case " + std::to_string(row) + " differs");}};
    if(kind == 0) {
      auto reach{file.integer<int32_t>()};
      auto const projectile{file.integer<int16_t>()};
      auto const radius{file.integer<int16_t>()};
      auto const centre{point(file)};
      bool const shorten{file.integer<uint8_t>() != 0};
      bool const expected_hit{file.integer<uint8_t>() != 0};
      auto const expected_reach{file.integer<int32_t>()};
      check(earthsiege::intersect_sphere(centre, radius, projectile, reach, shorten) == expected_hit);
      check(reach == expected_reach);
    } else if(kind == 1) {
      auto const name{file.cstring()};
      if(!definitions.contains(name)) definitions.emplace(name, earthsiege::read_collision_parts(resources.read("simvol0", "col/" + name + ".COL")));
      auto const part{file.integer<int16_t>()};
      auto const state{file.integer<int16_t>()};
      auto const matrix{transform(file)};
      auto reach{file.integer<int32_t>()};
      auto const projectile{file.integer<int16_t>()};
      auto const expected_hit{file.integer<int32_t>()};
      auto const expected_reach{file.integer<int32_t>()};
      auto const &definition{definitions.at(name)};
      auto const p{std::ranges::find(definition, part, &earthsiege::collision_part::model_part)};
      if(p == definition.end()) file.fail("fixture collision part");
      auto const s{std::ranges::find(p->states, state, &earthsiege::collision_state::index)};
      if(s == p->states.end()) file.fail("fixture collision state");
      auto const hit{earthsiege::intersect_spheres(s->spheres, matrix, projectile, reach)};
      check((hit ? *hit + 1 : 0) == expected_hit && reach == expected_reach);
    } else if(kind == 2) {
      auto const matrix{transform(file)};
      auto const reach{file.integer<int32_t>()};
      auto damage{file.integer<int16_t>()};
      earthsiege::shield_power shield{.front{file.integer<int16_t>()}, .rear{file.integer<int16_t>()}};
      auto const expected_hit{file.integer<int32_t>()};
      auto const expected_damage{file.integer<int16_t>()};
      auto const front{file.integer<int16_t>()};
      auto const rear{file.integer<int16_t>()};
      auto const hit{earthsiege::intercept_shield(shield, damage, 100, 1000, reach, matrix)};
      check((hit ? *hit + 1 : 0) == expected_hit && damage == expected_damage && shield.front == front && shield.rear == rear);
    } else file.fail("unknown hit-volume case");
  }
  file.require_end();
  if(count != 6608) file.fail("hit-volume reference coverage differs");
  std::cout << count << " original projectile hit-volume and shield cases agree" << std::endl;
  return EXIT_SUCCESS;
} catch(std::exception const &error) {
  std::cerr << error.what() << std::endl;
  return EXIT_FAILURE;
}
