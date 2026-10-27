#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include "resources/combat.h"
#include "resources/resource_store.h"

namespace {

class comparison {
  earthsiege::binary_reader &file;
  std::string name;
  size_t index{0};

public:
  comparison(earthsiege::binary_reader &source, std::string label) : file{source}, name{std::move(label)} {
  }
  void value(int64_t const actual) {
    auto const expected{file.integer<int32_t>()};
    if(actual != expected) throw std::runtime_error{name + " field " + std::to_string(index) + " differs"};
    ++index;
  }
  void component(earthsiege::damage_component const &part) {
    value(part.armour); value(part.debris); value(part.cell_sequence); value(part.parent); value(part.flags);
    value(static_cast<int64_t>(part.dependents.size()));
    for(auto const &dependency : part.dependents) {
      value(dependency.weight); value(dependency.component);
    }
  }
  void collision(earthsiege::collision_section const &state) {
    value(state.section); value(static_cast<int64_t>(state.spheres.size()));
    for(auto const &sphere : state.spheres) {
      for(unsigned int axis{0}; axis != 3; ++axis) value(sphere.centre[axis]);
      value(sphere.radius);
    }
  }
};

} // anonymous namespace

int main(int const argc, char **argv) try {
  if(argc != 3) return EXIT_FAILURE;
  earthsiege::resource_store resources{argv[1]};
  auto const bytes{earthsiege::read_file(argv[2])};
  earthsiege::binary_reader file{bytes};
  auto const magic{file.take(8)};
  if(std::string(magic.begin(), magic.end()) != "ESCOMBT1") file.fail("fixture version");
  auto const count{file.integer<uint16_t>()};
  for(unsigned int row{0}; row != count; ++row) {
    auto const kind{file.integer<uint8_t>()};
    auto const name{file.cstring()};
    earthsiege::binary_reader values{file.take(static_cast<size_t>(file.integer<uint32_t>()) * 4)};
    comparison expect{values, name};
    auto const data{resources.read("simvol0", name)};
    if(kind == 0) {
      auto const damage{earthsiege::read_damage_definition(data)};
      expect.value(static_cast<int64_t>(damage.hit_points.size()));
      for(auto const hp : damage.hit_points) expect.value(hp);
      expect.value(static_cast<int64_t>(damage.components.size()));
      for(auto const &part : damage.components) expect.component(part);
    } else if(kind == 1) {
      auto const parts{earthsiege::read_collision_parts(data)};
      expect.value(static_cast<int64_t>(parts.size()));
      for(auto const &part : parts) {
        expect.value(part.model_part); expect.value(static_cast<int64_t>(part.sections.size()));
        for(auto const &state : part.sections) expect.collision(state);
      }
    } else {
      auto const catalogue{earthsiege::read_weapon_catalogue(data)};
      expect.value(static_cast<int64_t>(catalogue.weapons.size()));
      for(auto const &weapon : catalogue.weapons) {
        expect.component(weapon.damage); expect.collision(weapon.collision);
        for(auto const model : weapon.model_indices) expect.value(model);
        expect.value(weapon.internal_maximum); expect.value(weapon.minimum_range); expect.value(weapon.range); expect.value(weapon.shot_value_penalty);
        for(auto const threshold : weapon.energy_thresholds) expect.value(threshold);
        expect.value(weapon.magazine); expect.value(weapon.barrels); expect.value(weapon.projectile);
        for(unsigned int axis{0}; axis != 3; ++axis) expect.value(weapon.muzzle_offset[axis]);
        for(unsigned int axis{0}; axis != 3; ++axis) expect.value(weapon.side_muzzle_offset[axis]);
        expect.value(weapon.refire_delay); expect.value(weapon.combat_rating); expect.value(weapon.damage_icon);
      }
      expect.value(static_cast<int64_t>(catalogue.projectiles.size()));
      for(auto const &projectile : catalogue.projectiles) {
        expect.value(static_cast<int16_t>(projectile.kind)); expect.value(projectile.subtype); expect.value(projectile.shield_damage); expect.value(projectile.armour_damage); expect.value(projectile.splash_q10); expect.value(projectile.speed);
        for(auto const effect : projectile.shield_effects) expect.value(effect);
        for(auto const effect : projectile.ground_effects) expect.value(effect);
        for(auto const effect : projectile.armour_effects) expect.value(effect);
      }
    }
    values.require_end();
  }
  file.require_end();
  bool rejected{false};
  try {
    earthsiege::read_damage_definition(resources.read("simvol0", "dmg/HOVTANK.DMG"));
  } catch(std::runtime_error const &) {
    rejected = true;
  }
  if(count != 36 || !rejected) throw std::runtime_error{"combat fixture coverage differs"};
  std::cout << count << " original-reader combat definitions agree; original HOVTANK layout exception is rejected" << std::endl;
  return EXIT_SUCCESS;
} catch(std::exception const &error) {
  std::cerr << error.what() << std::endl;
  return EXIT_FAILURE;
}
