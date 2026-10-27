#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include "resources/resource_store.h"
#include "simulation/herc_body.h"
#include "simulation/herc_loadout.h"
#include "simulation/hit_volumes.h"
#include "simulation/model_math.h"

namespace {

std::vector<int16_t> shorts(earthsiege::binary_reader &file) {
  std::vector<int16_t> result(file.integer<uint16_t>());
  for(auto &value : result) value = file.integer<int16_t>();
  return result;
}

vec3<int32_t> point(earthsiege::binary_reader &file) {
  return {file.integer<int32_t>(), file.integer<int32_t>(), file.integer<int32_t>()};
}

vec3<uint16_t> angles(earthsiege::binary_reader &file) {
  return {file.integer<uint16_t>(), file.integer<uint16_t>(), file.integer<uint16_t>()};
}

earthsiege::collision_sphere sphere(earthsiege::binary_reader &file) {
  return {{file.integer<int16_t>(), file.integer<int16_t>(), file.integer<int16_t>()}, file.integer<int16_t>()};
}

} // namespace

int main(int const argc, char **argv) try {
  if(argc != 3) return EXIT_FAILURE;
  earthsiege::resource_store resources{argv[1]};
  auto const weapons{earthsiege::read_weapon_catalogue(resources.read("simvol0", "dat/WEAPONS.DAT"))};
  auto const templates{earthsiege::read_model(resources.read("simvol0", "dts/MECHWPNS.DTS"))};
  auto const bytes{earthsiege::read_file(argv[2])};
  earthsiege::binary_reader file{bytes};
  auto const magic{file.take(8)};
  if(std::string(magic.begin(), magic.end()) != "ESCHIT01") file.fail("fixture version");
  auto const models{file.integer<uint16_t>()};
  unsigned int total{0}, hits{0};
  for(unsigned int model{0}; model != models; ++model) {
    auto const name{file.cstring()};
    auto const equipment{shorts(file)};
    auto const position{point(file)};
    auto const orientation{angles(file)};
    auto const definition{earthsiege::read_herc_definition(resources, name)};
    earthsiege::herc_body body{definition, weapons, templates, equipment, position, orientation};
    std::vector<earthsiege::collision_part> parts(file.integer<uint16_t>());
    for(auto &part : parts) {
      part.model_part = file.integer<int16_t>();
      part.sections.resize(file.integer<uint16_t>());
      for(auto &section : part.sections) {
        section.section = file.integer<int16_t>();
        auto const expected_bounds{sphere(file)};
        section.spheres.resize(file.integer<uint16_t>());
        for(auto &value : section.spheres) value = sphere(file);
        auto const bounds{earthsiege::collision_bounds(section.spheres)};
        if(bounds.centre != expected_bounds.centre || bounds.radius != expected_bounds.radius) file.fail(name + ": section bounds differ");
      }
    }
    auto const loadout{earthsiege::assemble_herc_loadout(definition, equipment, weapons)};
    if(loadout.collision != parts) file.fail(name + ": assembled collision differs");
    auto const steps{file.integer<uint16_t>()};
    for(unsigned int step{0}; step != steps; ++step) {
      earthsiege::herc_controls const controls{file.integer<int16_t>(), file.integer<int16_t>(), file.integer<int16_t>(), file.integer<int16_t>()};
      auto const ratio{file.integer<int16_t>()};
      body.step_requested(controls.throttle, controls.steering, {controls.torso, controls.elevation}, ratio);
      auto const cases{file.integer<uint16_t>()};
      for(unsigned int query{0}; query != cases; ++query) {
        auto transform{earthsiege::rotation_transform(angles(file))};
        transform.translation = point(file);
        auto const reach{file.integer<int32_t>()};
        auto const radius{file.integer<int16_t>()};
        auto const present{shorts(file)};
        bool const expected_hit{file.integer<uint8_t>() != 0};
        auto const distance{file.integer<int32_t>()};
        auto const section{file.integer<uint16_t>()};
        auto const hit{earthsiege::intersect_components(loadout.collision, present, definition.models.roots.front(), body.model().transforms(), transform, reach, radius)};
        if(hit.has_value() != expected_hit || (hit && (hit->distance + 1 != distance || hit->section != section))) file.fail(name + '/' + std::to_string(step) + '/' + std::to_string(query) + ": component selection differs");
        ++total;
        if(hit) ++hits;
      }
    }
  }
  file.require_end();
  if(models != 8 || total != 8192) file.fail("component-hit reference coverage differs");
  std::cout << total << " original articulated component queries agree, including " << hits << " hits" << std::endl;
  return EXIT_SUCCESS;
} catch(std::exception const &error) {
  std::cerr << error.what() << std::endl;
  return EXIT_FAILURE;
}
