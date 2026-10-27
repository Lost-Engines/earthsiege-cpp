#include "hit_dispatch.h"
#include <algorithm>
#include <stdexcept>
#include "simulation/fixed_math.h"
#include "simulation/model_math.h"

namespace earthsiege {
namespace {

vec3<int32_t> difference(vec3<int32_t> const &a, vec3<int32_t> const &b) {
  return {wrap_long(static_cast<int64_t>(a.x) - b.x), wrap_long(static_cast<int64_t>(a.y) - b.y), wrap_long(static_cast<int64_t>(a.z) - b.z)};
}

vec3<int32_t> ray_point(model_transform const &ray, int32_t const distance) {
  auto point{rotate_vector({0, distance, 0}, ray)};
  for(unsigned int axis{0}; axis != 3; ++axis) point[axis] = wrap_long(static_cast<int64_t>(point[axis]) + ray.translation[axis]);
  return point;
}

std::optional<size_t> nearby_companion(impact_world const &world, shot_source const &source) {
  auto const &owner{world.actors[source.actor]};
  int32_t best_distance{100000};
  std::optional<size_t> best;
  for(auto const index : source.group_members) {
    if(index >= world.actors.size()) throw std::out_of_range{"shot source group member"};
    auto const &candidate{world.actors[index]};
    if(index == source.actor || candidate.disabled || candidate.dead) continue;
    auto const distance{fast_distance(difference(candidate.position, owner.position))};
    if(distance < best_distance) { best_distance = distance; best = index; }
  }
  int32_t constexpr warning_distance{30000};
  return best_distance < warning_distance ? best : std::nullopt;
}

} // anonymous namespace

std::optional<int32_t> dispatch_impact(impact_request request, shot_source const &source, impact_world &world) {
  if(source.actor >= world.actors.size()) throw std::out_of_range{"projectile owner"};
  auto &owner{world.actors[source.actor]};
  bool hit{false};
  bool actor_hit{false};
  std::optional<impact_obstruction> obstruction;
  auto const end{ray_point(request.transform, request.reach)};
  if(end.z < 0) {
    auto const &origin{request.transform.translation};
    auto const delta{difference(end, origin)};
    auto const horizontal{fast_distance_2d({delta.x, delta.y})};
    auto const downward{wrap_word(-static_cast<int32_t>(direction_angle(horizontal, delta.z)))};
    if(origin.z < 0) request.reach = 0;
    else if(downward != 0) {
      auto const sine{model_cosine(static_cast<uint16_t>(downward - angle_quarter_turn))};
      if(sine == 0) throw std::runtime_error{"original ground intersection divides by zero"};
      request.reach = wrap_long(static_cast<int64_t>(origin.z) * model_matrix_one) / sine;
    }
    hit = true;
  }
  if(auto const scenery{intersect_scenery(world.scenery, request.transform, request.reach, request.radius)}) {
    request.reach = scenery->distance;
    obstruction = impact_obstruction{obstruction_kind::scenery, scenery->object};
    hit = true;
  }
  for(size_t index{0}; index != world.actors.size(); ++index) {
    auto &victim{world.actors[index]};
    if(index == source.actor || victim.waiting || !victim.hit) continue;
    auto const distance{victim.hit(request)};
    if(!distance) continue;
    // The original applies each hit during traversal, before it knows whether a later actor is nearer.
    bool const friendly{victim.side == owner.side};
    if(!friendly) {
      owner.hit_enemy = true;
      if(victim.trigger && !victim.quiet && world.events.trigger) world.events.trigger(*victim.trigger);
      obstruction.reset();
    } else obstruction = impact_obstruction{obstruction_kind::actor, index};
    if(!victim.dead && (friendly || (!victim.inactive && !victim.disabled))) {
      if(source.player && friendly && victim.group != owner.group && world.events.friendly_warning) {
        if(auto const companion{nearby_companion(world, source)}) world.events.friendly_warning(*companion);
      }
      if(world.events.react) world.events.react(index, source.actor, request.damage.armour);
    }
    request.reach = *distance;
    hit = true;
    actor_hit = true;
    int32_t constexpr near_hit_distance{500};
    if(*distance < near_hit_distance) break;
  }
  if(!hit) return std::nullopt;
  auto position{ray_point(request.transform, request.reach)};
  if(!actor_hit) {
    auto ground_position{position};
    ground_position.z = std::max(0, ground_position.z);
    auto const effect{request.damage.ground_effects[static_cast<size_t>(world.random.bounded(4))]};
    if(world.events.effect) world.events.effect(ground_position, effect);
  }
  if(obstruction && source.target && world.events.obstructed) {
    auto const delta{difference(position, owner.position)};
    auto const target_distance{fast_distance(difference(*source.target, owner.position))};
    if(fast_distance_2d({delta.x, delta.y}) < target_distance) {
      // This is a signed one-sided comparison in the original, not an absolute-angle cone.
      auto const error{wrap_word(static_cast<int32_t>(bearing_angle(owner.position, position)) - bearing_angle(owner.position, *source.target))};
      if(error < static_cast<int32_t>(angle_turn / 8)) world.events.obstructed(*obstruction);
    }
  }
  return request.reach;
}

} // namespace earthsiege
