#include "flyer_actor.h"
#include <algorithm>
#include <stdexcept>
#include <utility>
#include "simulation/fixed_math.h"
#include "resources/model_tree.h"
#include "simulation/hit_volumes.h"
#include "simulation/impact_damage.h"
#include "simulation/model_math.h"

namespace earthsiege {
namespace {
damage_state initial_damage(shape_part const &shape) {
  // Original memset fills both bytes of its presence word with one.
  return {{0}, {0}, {0x0101}, std::vector<int16_t>(shape.cell_frame_counts.size()), {}};
}
}
flyer_actor::flyer_actor(flyer_definition const &source, vec3<int32_t> const &position, vec3<uint16_t> const &angles)
  : definition{source}, visual{std::get<shape_part>(source.models.roots.at(0).content)},
    components{source.damage, initial_damage(std::get<shape_part>(source.models.roots.at(0).content))},
    flight{vec3<int32_t>{position},vec3<uint16_t>{angles},{0,source.profile.forward_step,0},{}} {}
flyer_profile const &flyer_actor::profile() const { return definition.profile; }
model_instance const &flyer_actor::instance() const { return visual; }
model_transform flyer_actor::placement() const { return flyer_placement(flight); }
flyer_motion &flyer_actor::motion() { return flight; }
flyer_motion const &flyer_actor::motion() const { return flight; }
vec3<uint16_t> const &flyer_actor::angles() const { return flight.angles; }
vec3<int16_t> const &flyer_actor::local_velocity() const { return flight.velocity; }
vec3<int16_t> const &flyer_actor::angular_velocity() const { return flight.angular_step; }
damage_state const &flyer_actor::damage() const { return components.state(); }
bool flyer_actor::dead() const { return destroyed; }
bool flyer_actor::is_detectable() const { return detectable; }
void flyer_actor::set_detectable(bool const value) { detectable = value; }
bool flyer_actor::direct_damage(int16_t const amount, random_state &random, flyer_damage_events const &events, bool const player_locked_target) {
  if(!events.explosion || !events.debris) throw std::invalid_argument{"flyer damage requires synchronous effect emitters"};
  auto const transform{placement()};
  components.apply_section(0, amount, [&](int16_t const count){ return random.bounded(count); }, [&](size_t const section){
    for(auto const &part : definition.collision) for(auto const &entry : part.sections) {
      if(entry.section != static_cast<int16_t>(section)) continue;
      auto const centre{collision_bounds(entry.spheres).centre};
      vec3<int32_t> position;
      if(part.model_part < 0) position = transform_point(centre,transform);
      else {
        auto const node{find_model_part(definition.models.roots.front(),part.model_part)};
        if(!node || node->header.transform < 0) throw std::out_of_range{"flyer damage section transform"};
        auto const local{transform_point(centre,visual.transforms()[static_cast<size_t>(node->header.transform)])};
        position = rotate_vector(local,transform);
        for(unsigned int axis{0}; axis != 3; ++axis) position[axis] = wrap_long(static_cast<int64_t>(position[axis]) + transform.translation[axis]);
      }
      bool const large{(definition.damage.components.at(section).flags & std::to_underlying(damage_section_flag::large_explosion)) != 0};
      events.explosion(large ? 17 : 10,position,random);
      events.debris(3,position,random);
      return;
    }
    throw std::out_of_range{"flyer damage section has no collision centre"};
  });
  std::ranges::copy(components.state().cells, visual.cell_frames().begin());
  if(components.state().present.front() != 0) return false;
  if(player_locked_target && events.locked_target_destroyed) events.locked_target_destroyed();
  if(events.kill_credit) events.kill_credit();
  destroyed = true;
  detectable = false;
  flight.position.z = -100000;                                       // original removal position; no falling/wreck phase for flyers
  if(events.destroyed) events.destroyed();
  return true;
}
std::optional<int32_t> flyer_actor::receive_impact(impact_request const &request, impact_conditions const &conditions, random_state &random, flyer_damage_events const &events, bool const player_locked_target) {
  if(!events.explosion || !events.debris) throw std::invalid_argument{"flyer impact requires synchronous effect emitters"};
  auto const transform{placement()};
  vec3<int32_t> delta;
  for(unsigned int axis{0}; axis != 3; ++axis) delta[axis] = wrap_long(static_cast<int64_t>(transform.translation[axis]) - request.transform.translation[axis]);
  auto const &root{definition.models.roots.front()};
  if(fast_distance(delta) > wrap_long(static_cast<int64_t>(request.reach) + request.radius + root.header.radius)) return std::nullopt;
  auto const actor_to_ray{compose_transforms(transform,inverse_transform(request.transform))};
  auto const hit{intersect_components(definition.collision,components.state().present,root,visual.transforms(),actor_to_ray,request.reach,request.radius)};
  if(!hit) return std::nullopt;
  auto effect{request.damage.armour_effects[static_cast<size_t>(random.next() & 3)]};
  auto const distance{wrap_long(static_cast<int64_t>(hit->distance) + 2)};       // component query and actor method each add one
  bool const killed{direct_damage(adjust_impact_damage(request.damage.armour,conditions),random,events,player_locked_target)};
  if(killed) effect = 10;
  auto position{rotate_vector({0,distance,0},request.transform)};
  for(unsigned int axis{0}; axis != 3; ++axis) position[axis] = wrap_long(static_cast<int64_t>(position[axis]) + request.transform.translation[axis]);
  events.explosion(effect,position,random);
  events.debris(killed ? 3 : 1,position,random);
  return distance;
}
scene_model flyer_actor::model() const { return {definition.models.roots,visual,placement(),6,definition.textures,{},{},{}}; }
} // namespace earthsiege
