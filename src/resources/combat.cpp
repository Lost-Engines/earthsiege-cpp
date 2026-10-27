#include "combat.h"
#include <utility>

namespace earthsiege {
namespace {

damage_component component(binary_reader &file) {
  damage_component result;
  result.armour = file.integer<int16_t>();
  result.debris = file.integer<int8_t>();
  result.cell_sequence = file.integer<int8_t>();
  result.parent = file.integer<int8_t>();
  result.flags = file.integer<uint8_t>();
  auto const count{file.integer<uint16_t>()};
  for(unsigned int index{0}; index != count; ++index) result.dependents.push_back(damage_dependency{.weight{file.integer<int16_t>()}, .component{file.integer<int16_t>()}});
  return result;
}

collision_section collision(binary_reader &file) {
  collision_section result;
  result.section = file.integer<int16_t>();
  auto const count{file.integer<uint16_t>()};
  for(unsigned int index{0}; index != count; ++index) result.spheres.push_back(collision_sphere{.centre{file.integer<int16_t>(), file.integer<int16_t>(), file.integer<int16_t>()}, .radius{file.integer<int16_t>()}});
  return result;
}

vec3<int16_t> vector(binary_reader &file) {
  return vec3<int16_t>{file.integer<int16_t>(), file.integer<int16_t>(), file.integer<int16_t>()};
}

} // anonymous namespace

damage_definition read_damage_definition(bytes const source) {
  binary_reader file{source, "damage definition"};
  damage_definition result;
  auto const health_count{file.integer<uint16_t>()};
  for(unsigned int index{0}; index != health_count; ++index) result.hit_points.push_back(file.integer<int16_t>());
  auto const component_count{file.integer<uint16_t>()};
  for(unsigned int index{0}; index != component_count; ++index) result.components.push_back(component(file));
  file.require_end();
  return result;
}

std::vector<collision_part> read_collision_parts(bytes const source) {
  binary_reader file{source, "collision parts"};
  std::vector<collision_part> result;
  auto const count{file.integer<uint16_t>()};
  for(unsigned int index{0}; index != count; ++index) {
    collision_part part;
    part.model_part = file.integer<int16_t>();
    auto const sections{file.integer<uint16_t>()};
    for(unsigned int section{0}; section != sections; ++section) part.sections.push_back(collision(file));
    result.push_back(std::move(part));
  }
  file.require_end();
  return result;
}

weapon_catalogue read_weapon_catalogue(bytes const source) {
  binary_reader file{source, "weapon catalogue"};
  weapon_catalogue result;
  /// WEAPONS.DAT has fixed record counts; its first short belongs to weapon zero
  for(auto &weapon : result.weapons) {
    weapon.damage = component(file);
    weapon.collision = collision(file);
    for(auto &model : weapon.model_indices) model = file.integer<int16_t>();
    weapon.internal_maximum = file.integer<int16_t>();
    weapon.minimum_range = file.integer<int32_t>();
    weapon.range = file.integer<int32_t>();
    weapon.shot_value_penalty = file.integer<int16_t>();
    for(auto &threshold : weapon.energy_thresholds) threshold = file.integer<int16_t>();
    weapon.magazine = file.integer<int16_t>();
    weapon.barrels = file.integer<int16_t>();
    weapon.projectile = file.integer<int16_t>();
    weapon.muzzle_offset = vector(file);
    weapon.side_muzzle_offset = vector(file);
    weapon.side_muzzle_offset.y = 0;                                            // original loader clears tail word 19
    weapon.refire_delay = file.integer<int16_t>();
    weapon.combat_rating = file.integer<int16_t>();
    weapon.damage_icon = file.integer<int16_t>();
  }
  for(auto &projectile : result.projectiles) {
    projectile.kind = static_cast<projectile_kind>(file.integer<int16_t>());
    projectile.subtype = file.integer<int16_t>();
    projectile.shield_damage = file.integer<int16_t>();
    projectile.armour_damage = file.integer<int16_t>();
    projectile.splash_q10 = file.integer<int16_t>();
    projectile.speed = file.integer<int16_t>();
    for(auto &effect : projectile.shield_effects) effect = file.integer<int16_t>();
    for(auto &effect : projectile.ground_effects) effect = file.integer<int16_t>();
    for(auto &effect : projectile.armour_effects) effect = file.integer<int16_t>();
  }
  file.require_end();
  return result;
}

} // namespace earthsiege
