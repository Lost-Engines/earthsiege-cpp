#include "ground_actor.h"
#include <type_traits>
#include <algorithm>
#include <stdexcept>
#include "simulation/hit_volumes.h"
#include "resources/model_tree.h"
#include "simulation/impact_damage.h"
#include "simulation/fixed_math.h"
#include "simulation/model_math.h"

namespace earthsiege {
void ground_actor::deploy(vec3<int32_t> const &position,vec3<uint16_t> const &angles){original_angles=angles;transform=rotation_transform(angles);transform.translation=position;}

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
    wreck{std::get<shape_part>(resources.model(selected, selected_side, true).content)}, transform{rotation_transform(angles)},original_angles{angles} {
  transform.translation = position;
  cell_timer = profile().cell_period;
  if(condition == 0) { accumulated_damage = profile().damage_capacity; destroyed = wreck_ready = true; }
  else if(condition > 0) accumulated_damage = wrap_word((100 - condition) * profile().damage_capacity / 100);
  section_damage[0]=accumulated_damage;
}

ground_profile const &ground_actor::profile() const { return catalogue.profile(type); }
int16_t ground_actor::damage() const { return accumulated_damage; }
bool ground_actor::dead() const { return destroyed; }
bool ground_actor::is_detectable() const { return !destroyed && (type == 5 || type == 6); }
bool ground_actor::is_turret() const { return type == 8; }
vec3<int32_t> ground_actor::target_position() const { return transform_point({0, 0, profile().target_height}, transform); }
std::optional<scenery_collider> ground_actor::obstruction() const {
  return wreck_ready || profile().collision.empty() ? std::optional<scenery_collider>{heightfield()} : std::nullopt;
}
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

void ground_actor::section_hit(size_t section,int16_t amount,random_state &random,ground_damage_events const &events){
  if(section==0){direct_damage(amount,random,events);return;}
  auto &damage{section_damage.at(section)};auto capacity{profile().expansion_sections.at(section-1)};
  if(static_cast<int>(damage)+amount>capacity){damage=capacity;section_present[section]=0;}
  else damage=wrap_word(damage+amount);
}
void ground_actor::direct_damage(int16_t const amount, random_state &random, ground_damage_events const &events) {
  if(!events.debris) throw std::invalid_argument{"ground damage requires a synchronous debris emitter"};
  bool const exceeded{profile().edition==game_edition::expansion?static_cast<int>(accumulated_damage)+amount>profile().damage_capacity:wrap_word(accumulated_damage+amount)>profile().damage_capacity};
  accumulated_damage = wrap_word(accumulated_damage + amount);section_damage[0]=accumulated_damage;
  if(!exceeded) return;                   // equality survives in the original
  accumulated_damage = profile().damage_capacity;section_damage[0]=accumulated_damage;
  if(events.kill_credit) events.kill_credit();
  destroyed = true;
  events.debris(3, transform.translation, random);
  if(events.destroyed) events.destroyed();                                     // group/counter/trigger consequences, in original order
  destruction_steps = 8;
}

std::optional<int32_t> ground_actor::receive_impact(impact_request const &request, impact_conditions const &conditions, random_state &random, ground_damage_events const &events) {
  if(!events.explosion || !events.debris) throw std::invalid_argument{"ground impact requires synchronous effect emitters"};
  std::optional<int32_t> distance;
  int16_t struck_section{};
  bool const heightfield_hit{wreck_ready||(profile().edition==game_edition::expansion?!profile().collision_enabled:profile().channels==0)};
  if(heightfield_hit) {
    std::array const candidates{heightfield()};
    if(auto const hit{intersect_scenery(candidates, request.transform, request.reach, request.radius)}) distance = wrap_long(static_cast<int64_t>(hit->distance) + 1);
  } else {
    vec3<int32_t> delta;
    for(unsigned int axis{0}; axis != 3; ++axis) delta[axis] = wrap_long(static_cast<int64_t>(transform.translation[axis]) - request.transform.translation[axis]);
    auto const &root{catalogue.model(type, side)};
    if(fast_distance(delta) > wrap_long(static_cast<int64_t>(request.reach) + request.radius + root.header.radius)) return std::nullopt;
    auto const actor_to_ray{compose_transforms(transform, inverse_transform(request.transform))};
    std::array<int16_t,1> const original_present{1};
    auto present{profile().edition==game_edition::expansion?std::span<int16_t const>{section_present}:std::span<int16_t const>{original_present}};
    if(auto const hit{intersect_components(profile().collision, present, root, instance().transforms(), actor_to_ray, request.reach, request.radius)}) {distance = wrap_long(static_cast<int64_t>(hit->distance) + 2);struck_section=hit->section;} // sphere consumer and base method each add one
  }
  if(!distance) return std::nullopt;
  auto position{rotate_vector({0, *distance, 0}, request.transform)};
  for(unsigned int axis{0}; axis != 3; ++axis) position[axis] = wrap_long(static_cast<int64_t>(position[axis]) + request.transform.translation[axis]);
  if(!destroyed) {
    auto raw{heightfield_hit&&request.damage.armour==0&&request.damage.shield==0?request.damage.plasma_armour.value_or(0):request.damage.armour};
    auto amount{adjust_impact_damage(raw, conditions,profile().edition)};
    auto const weak_point{transform_point(profile().weak_point, transform)};
    vec3<int32_t> delta;
    for(unsigned int axis{0}; axis != 3; ++axis) delta[axis] = wrap_long(static_cast<int64_t>(position[axis]) - weak_point[axis]);
    if(fast_distance(delta) < profile().weak_point_radius) amount = wrap_word(amount * 3);
    section_hit(static_cast<size_t>(std::max<int16_t>(0,struck_section)),amount,random,events);
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
  if(destroyed||type==10) return;
  if(type == 8) {
    advance_turret_cells(ratio);
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

turret_aim_state const &ground_actor::turret_aim() const { return aiming; }

vec3<int16_t> ground_actor::track_point(vec3<int32_t> const &position) {
  if(type != 8 || destroyed) throw std::logic_error{"tracking requires a live gun turret"};
  auto &rig{std::get<animation_rig>(intact)};
  auto const barrel{find_model_part(catalogue.model(type, side), 2)};
  if(!barrel || barrel->header.transform < 0) throw std::runtime_error{"gun turret has no barrel transform"};
  auto const local_point{[](vec3<int32_t> const &point, model_transform const &inverse){
    auto result{rotate_vector(point, inverse)};
    for(unsigned int axis{0}; axis != 3; ++axis) result[axis] = wrap_long(static_cast<int64_t>(result[axis]) + inverse.translation[axis]);
    return result;
  }};
  auto const local{local_point(local_point(position, inverse_transform(transform)), inverse_transform(rig.instance().transforms()[static_cast<size_t>(barrel->header.transform)]))};
  vec3<int16_t> const error{
    wrap_word(direction_angle(fast_distance_2d({local.x, local.y}), local.z)), 0,
    wrap_word(static_cast<int32_t>(direction_angle(local.x, local.y)) - angle_quarter_turn),
  };
  std::array<int, 2> const inputs{std::clamp(error.x >> 3, -256, 256), -std::clamp(error.z >> 3, -256, 256)};
  std::array<int16_t, 2> constexpr maximum_rate{2000, 2500}, acceleration{200, 800}, minimum_angle{-4000, -32768}, maximum_angle{4000, 32767};
  for(size_t axis{0}; axis != 2; ++axis) {
    auto const shaped{wrap_word((inputs[axis] * std::abs(inputs[axis]) + 128) >> 8)};
    auto const desired{wrap_word((maximum_rate[axis] * shaped + 128) >> 8)};
    auto &rate{aiming.rates[axis]};
    if(desired < rate) rate = std::max(desired, wrap_word(rate - acceleration[axis]));
    else if(desired > rate) rate = std::min(desired, wrap_word(rate + acceleration[axis]));
    aiming.angles[axis] = std::clamp(wrap_word(aiming.angles[axis] + rate), minimum_angle[axis], maximum_angle[axis]);
    rig.channel(axis).set_phase(static_cast<int16_t>(axis), static_cast<int16_t>(static_cast<uint16_t>(aiming.angles[axis]) >> 2));
  }
  for(size_t axis{2}; axis-- != 0;) { rig.channel(axis).rebase_motion(); rig.channel(axis).advance(100); }
  rig.update_pose();
  return vec3<int16_t>{error};
}

turret_fire_state const &ground_actor::turret_fire() const { return firing; }

void ground_actor::advance_turret_cells(frame_ratio const ratio) {
  // The unlit frame stops its own timer; firing restarts it.
  if(cell_timer == 0) return;
  cell_timer = std::max<int16_t>(0, wrap_word(cell_timer - ratio));
  if(cell_timer != 0) return;
  advance_cell();
  if(intact_instance().cell_frames()[static_cast<size_t>(profile().cell_sequence)] != 0) cell_timer = profile().cell_period;
}

void ground_actor::advance_turret(frame_ratio const ratio, weapon_catalogue const &weapons, turret_callbacks const &callbacks) {
  if(type != 8 || destroyed) throw std::logic_error{"turret control requires a live gun turret"};
  if(!callbacks.select || !callbacks.target || !callbacks.fire) throw std::invalid_argument{"turret control callbacks are incomplete"};
  auto const tick{[&](int16_t &timer){ timer = std::max<int16_t>(0, wrap_word(timer - ratio)); return timer == 0; }};
  if(tick(firing.target_timer)) {
    firing.target = callbacks.select();
    firing.target_timer = 10000;
  }
  advance_turret_cells(ratio);
  if(!firing.target) return;                                                  // burst/refire clocks freeze without a target
  auto const target{callbacks.target(*firing.target)};
  vec3<int32_t> delta;
  for(unsigned int axis{0}; axis != 3; ++axis) delta[axis] = wrap_long(static_cast<int64_t>(target.position[axis]) - transform.translation[axis]);
  auto const distance{fast_distance(delta)};
  vec3<int32_t> aim;
  for(unsigned int axis{0}; axis != 3; ++axis) aim[axis] = wrap_long(static_cast<int64_t>(target.position[axis]) + target.offset[axis]);
  int32_t constexpr maximum_range{40000};
  // The original uses projectile record 2's speed for lead, then looks up the
  // emitted bullet by kind/subtype. Keep those independently authored choices.
  auto const lead_speed{weapons.projectiles.at(2).speed};
  if(distance < maximum_range && lead_speed > 0 && target.speed != 0) {
    auto const lead{wrap_word(wrap_word(distance >> 3) * (target.speed * 8) / lead_speed)};
    model_transform forward{rotation_transform({0, 0, target.angles.z})};
    auto const movement{rotate_vector({0,lead,0},forward)};
    for(unsigned int axis{0}; axis != 3; ++axis) aim[axis] = wrap_long(static_cast<int64_t>(aim[axis]) + movement[axis]);
  }
  auto const error{track_point(aim)};
  if(tick(firing.burst_timer)) { firing.burst_phase = !firing.burst_phase; firing.burst_timer = 10000; }
  bool const ready{tick(firing.refire_timer)};
  if(!ready || !firing.burst_phase || error.x < -1000 || error.x >= 1000 || error.z < -1000 || error.z >= 1000 || distance >= maximum_range) return;
  auto const projectile{std::ranges::find_if(weapons.projectiles, [](projectile_definition const &p){ return p.kind == projectile_kind::bullet && p.subtype == 2; })};
  if(projectile == weapons.projectiles.end()) throw std::runtime_error{"turret bullet definition is missing"};
  auto const part{find_model_part(catalogue.model(type,side),2)};
  auto const muzzle_transform{compose_transforms(instance().transforms()[static_cast<size_t>(part->header.transform)],transform)};
  auto const angles{transform_angles(muzzle_transform)};
  for(int16_t const x : {int16_t{300}, int16_t{-300}}) {
    weapon_shot shot{.kind{shot_kind::bullet}, .projectile{static_cast<size_t>(projectile - weapons.projectiles.begin())}, .charge{-1}, .range{0},
      .muzzle{model_transform{muzzle_transform}, transform_point({x,400,0},muzzle_transform), vec3<uint16_t>{angles}}};
    callbacks.fire(shot);
  }
  firing.refire_timer = 1500;
  cell_timer = 1;
}

void ground_actor::advance_landing_craft(frame_ratio ratio,weapon_catalogue const &weapons,random_state &random,landing_callbacks const &callbacks){
  if(type!=10||destroyed)throw std::logic_error{"landing craft control requires a live landing craft"};
  auto tick=[&](int16_t &value){value=std::max<int16_t>(0,wrap_word(value-ratio));return value==0;};
  auto heading{static_cast<uint16_t>(original_angles.z+0x1555)};
  for(size_t b{};b<3;++b,heading=static_cast<uint16_t>(heading+0x5554)){
    auto &bank{landing_banks_[b]};
    if(tick(bank.target_timer)){bank.target=callbacks.select(heading);bank.target_timer=10000;}
    if(!bank.target)continue;
    auto const target{callbacks.target(*bank.target)};vec3<int32_t> delta,aim;
    for(unsigned a{};a<3;++a){delta[a]=wrap_long(static_cast<int64_t>(target.position[a])-transform.translation[a]);aim[a]=wrap_long(static_cast<int64_t>(target.position[a])+target.offset[a]-transform.translation[a]);}
    auto distance{fast_distance(delta)};auto pitch{wrap_word(direction_angle(fast_distance_2d({aim.x,aim.y}),aim.z))};auto yaw{wrap_word(static_cast<int32_t>(direction_angle(aim.x,aim.y))-angle_quarter_turn-heading)};
    for(size_t w{};w<3;++w){auto section{b*2+1+(w!=0)};auto capacity{profile().expansion_sections.at(section-1)};
      if(section_damage[section]==capacity){if(w==0)intact_instance().cell_frames()[b==2?1:b+2]=1;continue;}
      auto &state{bank.weapons[w]};if(tick(state.phase_timer)){state.phase=!state.phase;state.phase_timer=wrap_word(5000+random.bounded(5000));}
      auto const &definition{catalogue.landing_weapons()[w]};
      if(!tick(state.refire)||!state.phase||static_cast<uint16_t>(yaw+definition.yaw_limit)>=static_cast<uint16_t>(definition.yaw_limit*2)||static_cast<uint16_t>(pitch+definition.pitch_limit)>=static_cast<uint16_t>(definition.pitch_limit*2)||distance>=definition.range)continue;
      auto placement{rotation_transform({0,0,heading})};placement.translation=transform.translation;
      auto position{rotate_vector(definition.muzzle,placement)};for(unsigned a{};a<3;++a)position[a]=wrap_long(static_cast<int64_t>(position[a])+transform.translation[a]);vec3<uint16_t> angles{0,0,heading};weapon_shot shot;
      if(w==0){shot.kind=shot_kind::rocket;auto found{std::ranges::find_if(weapons.projectiles,[](auto const &p){return p.kind==projectile_kind::rocket&&p.subtype==3;})};if(found==weapons.projectiles.end())throw std::runtime_error{"landing craft missile definition"};shot.projectile=static_cast<size_t>(found-weapons.projectiles.begin());}
      else {for(unsigned a{};a<3;++a)aim[a]=wrap_long(static_cast<int64_t>(target.position[a])+target.offset[a]-position[a]);angles={direction_angle(fast_distance_2d({aim.x,aim.y}),aim.z),0,static_cast<uint16_t>(direction_angle(aim.x,aim.y)-angle_quarter_turn)};placement=rotation_transform(angles);placement.translation=position;shot.kind=shot_kind::beam;shot.projectile=static_cast<size_t>(weapons.weapons.at(6).projectile);shot.charge=weapons.weapons.at(6).energy_thresholds[1];shot.range=weapons.weapons.at(6).range;}
      shot.muzzle={model_transform{placement},vec3<int32_t>{position},vec3<uint16_t>{angles}};callbacks.fire(shot,*bank.target);state.refire=definition.refire;
    }
  }
}

} // namespace earthsiege
