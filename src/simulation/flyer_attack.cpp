#include "flyer_attack.h"
#include <algorithm>
#include <stdexcept>
#include "simulation/fixed_math.h"
#include "simulation/model_math.h"

namespace earthsiege {
namespace {
vec3<int32_t> difference(vec3<int32_t> const &a, vec3<int32_t> const &b) {
  return {wrap_long(static_cast<int64_t>(a.x)-b.x),wrap_long(static_cast<int64_t>(a.y)-b.y),wrap_long(static_cast<int64_t>(a.z)-b.z)};
}
int16_t elevation(vec3<int32_t> const &from, vec3<int32_t> const &to) {
  auto const delta{difference(to,from)}; return wrap_word(direction_angle(fast_distance_2d({delta.x,delta.y}),delta.z));
}
void approach(int16_t &value, int16_t const target, int16_t const step) {
  if(target < value) { value = wrap_word(value-step); if(value <= target) value = target; }
  else if(value < target) { value = wrap_word(value+step); if(target <= value) value = target; }
}
}
int16_t flyer_reported_speed(flyer_motion const &flight) { return wrap_word((flight.velocity.y * 1000 + 512) >> 10); }
void attack_with_flyer(flyer_motion &flight, flyer_attack_state &state, flyer_profile const &profile, flyer_attack_target const &target, weapon_catalogue const &weapons, frame_ratio const ratio, random_state &random, std::function<void(weapon_shot const &)> const &fire) {
  auto aim{target.position};
  auto bearing{wrap_word(bearing_angle(flight.position,aim)-flight.angles.z)};
  auto const delta{difference(aim,flight.position)};
  auto const distance{fast_distance_2d({delta.x,delta.y})};
  auto speed{flyer_reported_speed(flight)};
  if(state.phase == flyer_attack_phase::approaching && distance < 45000) speed = weapons.projectiles.at(2).speed;
  if(speed > 0 && target.speed != 0) {
    auto const lead{wrap_word(wrap_long(static_cast<int64_t>(wrap_word(std::min(distance,200000) >> 3)) * (target.speed * 8)) / speed)};
    auto const offset{rotate_vector({0,lead,0},rotation_transform({0,0,target.heading}))};
    for(unsigned int axis{0}; axis != 3; ++axis) aim[axis] = wrap_long(static_cast<int64_t>(aim[axis])+offset[axis]);
    bearing = wrap_word(bearing_angle(flight.position,aim)-flight.angles.z);
  }
  switch(state.phase) {
    case flyer_attack_phase::departing:
      bearing = wrap_word(bearing-32768);
      state.missile_latch = 0;
      if(distance > 80000) state.phase = flyer_attack_phase::approaching;
      hold_flyer_altitude(flight,profile,10000);
      break;
    case flyer_attack_phase::approaching:
      if(distance < 45000) {
        if(!flight.obstacle) aim_flyer_pitch(flight,profile,elevation(flight.position,aim));
        else hold_flyer_altitude(flight,profile,0);
        if(!state.firing_window) { state.firing_window = true; random.next(); }
        if(bearing < -8192 || bearing >= 8192 || flight.position.z < 5500) { state.firing_window = false; state.phase = flyer_attack_phase::departing; }
      }
      break;
    case flyer_attack_phase::circling:
      state.requested_speed = distance < 8000 ? -100 : distance <= 20000 ? 0 : 100;
      steer_flyer(flight,profile,0,wrap_word(-static_cast<int32_t>(wrap_word(flight.angles.y)) >> 4));
      aim_flyer_pitch(flight,profile,-5000);
      if(wrap_word(flight.angles.y) >= -1000 && wrap_word(flight.angles.y) < 1000) {
        flight.angular_step.z = wrap_word(((bearing >> 4) * profile.source[8] + 128) >> 8);
        approach(flight.velocity.x,500,40);
      }
      state.manoeuvre_timer = std::max(0,wrap_long(static_cast<int64_t>(state.manoeuvre_timer)-ratio));
      if(state.manoeuvre_timer == 0) { state.phase = flyer_attack_phase::levelling; state.contact_state = 0; }
      break;
    case flyer_attack_phase::levelling:
      steer_flyer(flight,profile,0,wrap_word(-static_cast<int32_t>(wrap_word(flight.angles.y)) >> 4));
      aim_flyer_pitch(flight,profile,0);
      approach(flight.angular_step.z,0,profile.bank_acceleration);
      approach(flight.velocity.x,0,40);
      if(flight.velocity.x == 0) state.phase = flyer_attack_phase::departing;
      break;
  }
  if(state.phase != flyer_attack_phase::departing && distance < 45000 && bearing >= -1000 && bearing < 1000) {
    auto const pitch_error{wrap_word(elevation(flight.position,aim)-flight.angles.x)};
    if(pitch_error >= -1000 && pitch_error < 1000 && flight.angular_step.x >= -10 && flight.angular_step.x < 10) {
      state.refire_timer = std::max<int16_t>(0,wrap_word(state.refire_timer-ratio));
      if(state.refire_timer == 0) {
        bool const rocket{state.missile_latch == 0 && distance < 30000};
        auto const kind{rocket ? projectile_kind::rocket : projectile_kind::bullet}; auto const subtype{rocket ? 0 : 2};
        auto const projectile{std::ranges::find_if(weapons.projectiles,[&](projectile_definition const &p){return p.kind == kind && p.subtype == subtype;})};
        if(projectile == weapons.projectiles.end()) throw std::runtime_error{"flyer projectile definition is missing"};
        auto const transform{flyer_placement(flight)};
        for(int16_t const x : {int16_t{500},int16_t{-500}}) {
          weapon_shot shot{.kind{rocket ? shot_kind::rocket : shot_kind::bullet},.projectile{static_cast<size_t>(projectile-weapons.projectiles.begin())},.charge{-1},.range{0},
            .muzzle{model_transform{transform},transform_point({x,200,-100},transform),vec3<uint16_t>{flight.angles}},.inherited_speed{rocket ? flight.velocity.y : int16_t{0}}};
          fire(shot);
          if(rocket) { state.missile_latch = 1; break; }
        }
        state.refire_timer = 1500;
      }
    }
  }
  if(static_cast<int16_t>(state.phase) < 2) { state.requested_speed = 180; turn_flyer(flight,profile,bearing); }
}
} // namespace earthsiege
