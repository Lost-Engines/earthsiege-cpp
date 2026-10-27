#include "hit_volumes.h"
#include <algorithm>
#include <stdexcept>
#include "resources/model_tree.h"
#include "simulation/fixed_math.h"
#include "simulation/model_math.h"

namespace earthsiege {

bool intersect_sphere(vec3<int32_t> const &centre, int16_t const radius, int16_t const projectile_radius, int32_t &reach, bool const shorten) {
  if(static_cast<uint32_t>(centre.y) >= static_cast<uint32_t>(reach)) return false;
  auto const lateral{fast_distance_2d({centre.x, centre.z})};
  auto const combined{wrap_word(radius + projectile_radius)};
  // The unsigned word interval is deliberate: it also retains overflowing radii.
  if(lateral >= 32767 || static_cast<uint16_t>(lateral + combined) >= static_cast<uint16_t>(combined * 2)) return false;
  if(shorten) reach = std::max(0, wrap_long(static_cast<int64_t>(centre.y) - (combined - lateral)));
  return true;
}

std::optional<int32_t> intersect_spheres(std::span<collision_sphere const> const spheres, model_transform const &object_to_ray, int16_t const projectile_radius, int32_t &reach, vec3<int16_t> const &offset) {
  bool hit{false};
  for(auto const &sphere : spheres) {
    vec3<int16_t> const point{wrap_word(sphere.centre.x + offset.x), wrap_word(sphere.centre.y + offset.y), wrap_word(sphere.centre.z + offset.z)};
    if(intersect_sphere(transform_point(point, object_to_ray), sphere.radius, projectile_radius, reach, true)) hit = true;
  }
  return hit ? std::optional{reach} : std::nullopt;
}

collision_sphere collision_bounds(std::span<collision_sphere const> const spheres) {
  vec3<int16_t> low{10000, 10000, 10000}, high{-10000, -10000, -10000};
  for(auto const &sphere : spheres) {
    for(unsigned int axis{0}; axis != 3; ++axis) {
      high[axis] = wrap_word(std::max<int>(high[axis], sphere.centre[axis] + sphere.radius));
      low[axis] = wrap_word(std::min<int>(low[axis], sphere.centre[axis] - sphere.radius));
    }
  }
  collision_sphere result;
  vec3<int32_t> extent;
  for(unsigned int axis{0}; axis != 3; ++axis) {
    result.centre[axis] = static_cast<int16_t>(wrap_word(low[axis] + high[axis]) >> 1);
    extent[axis] = wrap_word(high[axis] - result.centre[axis]);
  }
  result.radius = wrap_word(fast_distance(extent));
  return result;
}

std::optional<component_hit> intersect_components(std::span<collision_part const> const parts, std::span<int16_t const> const present, model_part const &model, std::span<model_transform const> const transforms, model_transform const &actor_to_ray, int32_t reach, int16_t const projectile_radius) {
  std::optional<component_hit> hit;
  for(auto const &part : parts) {
    auto transform{actor_to_ray};
    if(part.model_part >= 0) {
      auto const node{find_model_part(model, part.model_part)};
      if(!node || node->header.transform < 0 || static_cast<size_t>(node->header.transform) >= transforms.size()) throw std::out_of_range{"collision model part transform"};
      transform = compose_transforms(transforms[static_cast<size_t>(node->header.transform)], actor_to_ray);
    }
    for(auto const &section : part.sections) {
      auto const index{static_cast<size_t>(section.section)};
      if(index >= present.size()) throw std::out_of_range{"collision damage section"};
      if(!present[index]) continue;
      auto const bounds{collision_bounds(section.spheres)};
      if(!intersect_sphere(transform_point(bounds.centre, transform), bounds.radius, projectile_radius, reach, false)) continue;
      if(auto const depth{intersect_spheres(section.spheres, transform, projectile_radius, reach)}) hit = component_hit{*depth, section.section};
    }
  }
  return hit;
}

std::optional<int32_t> intercept_shield(shield_power &shield, int16_t &damage, int16_t const centre_height, int16_t const radius, int32_t const reach, model_transform const &actor_to_ray) {
  auto const centre{transform_point({0, 0, centre_height}, actor_to_ray)};
  auto const lateral{wrap_word(fast_distance_2d({centre.x, centre.z}))};
  if(static_cast<uint32_t>(centre.y) >= static_cast<uint32_t>(reach) || static_cast<uint16_t>(lateral + radius) >= static_cast<uint16_t>(radius * 2)) return std::nullopt;
  auto const ray_origin{inverse_transform(actor_to_ray).translation};
  auto &half{ray_origin.y > 0 ? shield.front : shield.rear};
  auto const absorbed{std::min(damage, half)};
  half = wrap_word(half - absorbed);
  damage = wrap_word(damage - absorbed);
  return std::max(0, wrap_long(static_cast<int64_t>(centre.y) - wrap_word(radius - lateral)));
}

} // namespace earthsiege
