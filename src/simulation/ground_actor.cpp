#include "ground_actor.h"
#include <type_traits>
#include <algorithm>
#include <stdexcept>
#include "simulation/hit_volumes.h"
#include "simulation/impact_damage.h"
#include "simulation/fixed_math.h"
#include "simulation/model_math.h"

namespace earthsiege {
namespace {

std::variant<model_instance, animation_rig> make_body(ground_catalogue const &catalogue, size_t const type, uint8_t const side) {
  auto const &shape{std::get<shape_part>(catalogue.model(type, side).content)};
  if(catalogue.profile(type).channels == 0) return model_instance{shape};
  animation_rig rig{shape};
  for(int16_t index{0}; index != catalogue.profile(type).channels; ++index) {
    auto const slot{rig.add_channel(index)};
    rig.channel(slot).set_rate(catalogue.profile(type).animation_rates.at(static_cast<size_t>(index)));
    if(index == 1) rig.channel(slot).set_phase(1, 6000);                       // original initial elevation phase, in Q14
  }
  return rig;
}

} // namespace

ground_actor::ground_actor(ground_catalogue const &resources, size_t const selected, uint8_t const selected_side, int16_t const condition, vec3<int32_t> const &position, vec3<uint16_t> const &angles)
  : catalogue{resources}, type{selected}, side{selected_side}, intact{make_body(resources, selected, selected_side)},
    wreck{std::get<shape_part>(resources.model(selected, selected_side, true).content)}, transform{rotation_transform(angles)} {
  transform.translation = position;
  cell_timer = profile().cell_period;
  if(condition == 0) { accumulated_damage = profile().damage_capacity; destroyed = wreck_ready = true; }
  else if(condition > 0) accumulated_damage = wrap_word((100 - condition) * profile().damage_capacity / 100);
}

ground_profile const &ground_actor::profile() const { return catalogue.profile(type); }
int16_t ground_actor::damage() const { return accumulated_damage; }
bool ground_actor::dead() const { return destroyed; }
model_instance const &ground_actor::instance() const {
  if(wreck_ready) return wreck;
  return std::visit([](auto const &body) -> model_instance const & {
    if constexpr(std::is_same_v<std::decay_t<decltype(body)>, animation_rig>) return body.instance();
    else return body;
  }, intact);
}
model_transform const &ground_actor::placement() const { return transform; }
scene_model ground_actor::model() const {
  auto const &root{catalogue.model(type, side, wreck_ready)};
  return {std::span{&root, 1}, instance(), model_transform{transform}, 5, catalogue.textures(), {}, {}, {}};
}
std::optional<scenery_collider> ground_actor::occupancy() const {
  if(profile().channels != 0) return std::nullopt;
  return heightfield();
}
scenery_collider ground_actor::heightfield() const {
  auto const &root{catalogue.model(type, side, wreck_ready)};
  auto const &shape{std::get<shape_part>(root.content)};
  return scenery_collider{shape.collision ? &*shape.collision : nullptr, model_transform{transform}, root.header.radius};
}

bool ground_actor::is_wreck() const { return wreck_ready; }
int16_t ground_actor::remaining_bursts() const { return destruction_steps; }
int16_t ground_actor::burst_timer() const { return destruction_timer; }
int16_t ground_actor::cell_clock() const { return cell_timer; }
model_instance &ground_actor::intact_instance() {
  return std::visit([](auto &body) -> model_instance & {
    if constexpr(std::is_same_v<std::decay_t<decltype(body)>, animation_rig>) return body.instance();
    else return body;
  }, intact);
}

void ground_actor::direct_damage(int16_t const amount, random_state &random, ground_damage_events const &events) {
  if(!events.debris) throw std::invalid_argument{"ground damage requires a synchronous debris emitter"};
  accumulated_damage = wrap_word(accumulated_damage + amount);
  if(accumulated_damage <= profile().damage_capacity) return;                   // equality survives in the original
  accumulated_damage = profile().damage_capacity;
  if(events.kill_credit) events.kill_credit();
  destroyed = true;
  events.debris(3, transform.translation, random);
  if(events.destroyed) events.destroyed();                                     // group/counter/trigger consequences, in original order
  destruction_steps = 8;
}

std::optional<int32_t> ground_actor::receive_impact(impact_request const &request, impact_conditions const &conditions, random_state &random, ground_damage_events const &events) {
  if(!events.explosion || !events.debris) throw std::invalid_argument{"ground impact requires synchronous effect emitters"};
  std::optional<int32_t> distance;
  if(wreck_ready || profile().channels == 0) {
    std::array const candidates{heightfield()};
    if(auto const hit{intersect_scenery(candidates, request.transform, request.reach, request.radius)}) distance = wrap_long(static_cast<int64_t>(hit->distance) + 1);
  } else {
    vec3<int32_t> delta;
    for(unsigned int axis{0}; axis != 3; ++axis) delta[axis] = wrap_long(static_cast<int64_t>(transform.translation[axis]) - request.transform.translation[axis]);
    auto const &root{catalogue.model(type, side)};
    if(fast_distance(delta) > wrap_long(static_cast<int64_t>(request.reach) + request.radius + root.header.radius)) return std::nullopt;
    auto const actor_to_ray{compose_transforms(transform, inverse_transform(request.transform))};
    std::array<int16_t, 1> const present{1};                                   // bases have a single damage section
    if(auto const hit{intersect_components(profile().collision, present, root, instance().transforms(), actor_to_ray, request.reach, request.radius)}) distance = wrap_long(static_cast<int64_t>(hit->distance) + 2); // sphere consumer and base method each add one
  }
  if(!distance) return std::nullopt;
  auto position{rotate_vector({0, *distance, 0}, request.transform)};
  for(unsigned int axis{0}; axis != 3; ++axis) position[axis] = wrap_long(static_cast<int64_t>(position[axis]) + request.transform.translation[axis]);
  if(!destroyed) {
    auto amount{adjust_impact_damage(request.damage.armour, conditions)};
    auto const weak_point{transform_point(profile().weak_point, transform)};
    vec3<int32_t> delta;
    for(unsigned int axis{0}; axis != 3; ++axis) delta[axis] = wrap_long(static_cast<int64_t>(position[axis]) - weak_point[axis]);
    if(fast_distance(delta) < profile().weak_point_radius) amount = wrap_word(amount * 3);
    direct_damage(amount, random, events);
  }
  events.explosion(request.damage.armour_effects.at(static_cast<size_t>(random.next() & 3)), position, random);
  events.debris(1, position, random);
  return distance;
}

void ground_actor::advance_cell() {
  auto const sequence{static_cast<size_t>(profile().cell_sequence)};
  auto &cell{intact_instance().cell_frames()[sequence]};
  auto const &shape{std::get<shape_part>(catalogue.model(type, side).content)};
  cell = static_cast<uint16_t>((cell + 1) % shape.cell_frame_counts.at(sequence));
}

void ground_actor::advance(frame_ratio const ratio, random_state &random, ground_damage_events const &events) {
  auto const tick{[&](int16_t &timer){ timer = std::max<int16_t>(0, wrap_word(timer - ratio)); return timer == 0; }};
  if(destruction_steps != 0 && tick(destruction_timer)) {
    --destruction_steps;
    if(destruction_steps == 0) {
      wreck_ready = true;
      if(events.attached_smoke) events.attached_smoke(profile().destruction_centre);
    } else {
      if(!events.explosion) throw std::invalid_argument{"base destruction requires a synchronous explosion emitter"};
      if(destruction_steps == 1) {
        events.explosion(16, transform_point(profile().destruction_centre, transform), random);
        destruction_timer = 2000;
      } else {
        vec3<int32_t> local;
        for(unsigned int axis{0}; axis != 3; ++axis) local[axis] = random.bounded(wrap_word(profile().burst_half_extent[axis] * 2)) - profile().burst_half_extent[axis] + profile().burst_centre[axis];
        auto position{rotate_vector(local, transform)};
        for(unsigned int axis{0}; axis != 3; ++axis) position[axis] = wrap_long(static_cast<int64_t>(position[axis]) + transform.translation[axis]);
        events.explosion(15, position, random);
        destruction_timer = 300;
      }
    }
  }
  if(destroyed) return;
  if(type == 8) {
    // Turret muzzle cells stop when they reach the unlit frame. Firing restarts
    // this timer; target tracking and shot production are a separate consumer.
    if(cell_timer != 0 && tick(cell_timer)) {
      advance_cell();
      if(intact_instance().cell_frames()[static_cast<size_t>(profile().cell_sequence)] != 0) cell_timer = profile().cell_period;
    }
    return;
  }
  if(profile().cell_sequence >= 0 && tick(cell_timer)) { cell_timer = profile().cell_period; advance_cell(); }
  if(auto *rig{std::get_if<animation_rig>(&intact)}) {
    // Fixed bases advance by 100 animation units per simulation dispatch; the
    // cell/explosion clocks instead use the global frame ratio.
    for(int index{profile().channels}; index-- != 0;) { rig->channel(static_cast<size_t>(index)).rebase_motion(); rig->channel(static_cast<size_t>(index)).advance(100); }
    rig->update_pose();
  }
}

} // namespace earthsiege
