#pragma once
#include <optional>
#include <span>
#include "resources/combat.h"
#include "resources/model.h"
#include "simulation/shield_power.h"

namespace earthsiege {

/// Octagonal cross-section and strict centre/range bounds, as used by the original game
bool intersect_sphere(vec3<int32_t> const &centre, int16_t radius, int16_t projectile_radius, int32_t &reach, bool shorten);
/// Retain stored sphere order, shortening the ray immediately after each accepted hit
std::optional<int32_t> intersect_spheres(std::span<collision_sphere const> spheres, model_transform const &object_to_ray, int16_t projectile_radius, int32_t &reach, vec3<int16_t> const &offset = {});
struct component_hit {
  int32_t distance{0};
  int16_t section{0};
};

collision_sphere collision_bounds(std::span<collision_sphere const> spheres);
/// Follow articulated part transforms and section presence; retain original stored traversal order
std::optional<component_hit> intersect_components(std::span<collision_part const> parts, std::span<int16_t const> present, model_part const &model, std::span<model_transform const> transforms, model_transform const &actor_to_ray, int32_t reach, int16_t projectile_radius);

/// Shield origin/extent are chassis properties; an empty shield still reports a volume hit
std::optional<int32_t> intercept_shield(shield_power &shield, int16_t &damage, int16_t centre_height, int16_t radius, int32_t reach, model_transform const &actor_to_ray);

} // namespace earthsiege
