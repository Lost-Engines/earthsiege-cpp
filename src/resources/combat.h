#pragma once
#include <array>
#include <cstdint>
#include <vector>
#include "resources/binary_reader.h"
#include "vectorstorm/vector/vector3.h"

namespace earthsiege {

struct damage_dependency {
  int16_t weight{0};
  int16_t component{0};
  bool operator==(damage_dependency const &) const = default;
};

enum class damage_section_flag : uint8_t {
  detachable = 1U << 0,
  attached_effect = 1U << 1,
  large_explosion = 1U << 2,
};

struct damage_component {
  int16_t armour{0};
  int8_t debris{0};
  int8_t cell_sequence{0};
  int8_t parent{0};
  uint8_t flags{0};
  std::vector<damage_dependency> dependents;
  bool operator==(damage_component const &) const = default;
};

struct collision_sphere {
  vec3<int16_t> centre;
  int16_t radius{0};
  bool operator==(collision_sphere const &) const = default;
};

struct collision_section {
  int16_t section{0};
  std::vector<collision_sphere> spheres;
  bool operator==(collision_section const &) const = default;
};

struct collision_part {
  int16_t model_part{0};
  std::vector<collision_section> sections;
  bool operator==(collision_part const &) const = default;
};

struct damage_definition {
  std::vector<int16_t> hit_points;
  std::vector<damage_component> components;
  bool operator==(damage_definition const &) const = default;
};

struct weapon_definition {
  damage_component damage;
  collision_section collision;
  std::array<int16_t, 4> model_indices{};
  int16_t internal_maximum{0};
  int32_t minimum_range{0};
  int32_t range{0};
  int16_t shot_value_penalty{0};
  std::array<int16_t, 2> energy_thresholds{};
  int16_t magazine{0};
  int16_t barrels{0};
  int16_t projectile{0};
  vec3<int16_t> muzzle_offset;
  vec3<int16_t> side_muzzle_offset;
  int16_t refire_delay{0};
  int16_t combat_rating{0};
  int16_t damage_icon{0};
};

enum class projectile_kind : int16_t {
  rocket = 0,
  bullet = 2,
  grenade = 3,
  beam = 4,
};

struct projectile_definition {
  projectile_kind kind{projectile_kind::rocket};
  int16_t subtype{0};
  int16_t shield_damage{0};
  int16_t armour_damage{0};
  int16_t splash_q10{0};
  int16_t speed{0};
  std::array<int16_t, 4> shield_effects{};
  std::array<int16_t, 4> ground_effects{};
  std::array<int16_t, 4> armour_effects{};
};

inline int16_t constexpr empty_weapon_slot{17};
inline size_t constexpr original_weapon_count{17};
inline size_t constexpr original_projectile_count{19};

struct weapon_catalogue {
  std::array<weapon_definition, original_weapon_count> weapons;
  std::array<projectile_definition, original_projectile_count> projectiles;
};

damage_definition read_damage_definition(bytes source);
std::vector<collision_part> read_collision_parts(bytes source);
std::vector<collision_part> read_collision_parts(binary_reader &source);
weapon_catalogue read_weapon_catalogue(bytes source);

} // namespace earthsiege
