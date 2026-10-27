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
/// Shield origin/extent are chassis properties; an empty shield still reports a volume hit
std::optional<int32_t> intercept_shield(shield_power &shield, int16_t &damage, int16_t centre_height, int16_t radius, int32_t reach, model_transform const &actor_to_ray);

} // namespace earthsiege
