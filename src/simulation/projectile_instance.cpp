#include "projectile_instance.h"
#include <algorithm>
#include <stdexcept>
#include "simulation/fixed_math.h"
#include "simulation/model_math.h"

namespace earthsiege {
namespace {

model_part const &projectile_shape(projectile_definition const &definition, projectile_motions const &motions, model_resource const &bullets, model_resource const &rockets) {
  auto const subtype{static_cast<size_t>(definition.subtype)};
  if(definition.kind == projectile_kind::bullet) return bullets.roots.at(static_cast<size_t>(motions.bullets.at(subtype).model));
  if(definition.kind == projectile_kind::rocket) return rockets.roots.at(static_cast<size_t>(motions.rockets.at(subtype).model));
  throw std::invalid_argument{"projectile has no bullet/rocket model"};
}

} // anonymous namespace

impact_payload projectile_payload(projectile_definition const &definition, int16_t const charge) {
  impact_payload result{
    .armour{definition.armour_damage}, .shield{definition.shield_damage}, .splash_q10{definition.splash_q10},
    .shield_effects{definition.shield_effects}, .ground_effects{definition.ground_effects}, .armour_effects{definition.armour_effects},
    .missile_subtype{std::nullopt},
  };
  if(definition.kind == projectile_kind::rocket) result.missile_subtype = static_cast<uint8_t>(definition.subtype);
  else if(definition.kind == projectile_kind::beam || charge != 0) {
    // Zero is the ordinary bullet's unscaled sentinel; beams always multiply, even at zero.
    result.armour = wrap_word((result.armour * charge + 512) >> 10);
    result.shield = wrap_word((result.shield * charge + 512) >> 10);
  }
  return result;
}

beam_path dispatch_beam(weapon_shot const &shot, weapon_catalogue const &definitions, impact_query const &hit) {
  if(shot.kind != shot_kind::beam) throw std::invalid_argument{"non-beam spawn request"};
  auto const &definition{definitions.projectiles.at(shot.projectile)};
  int16_t constexpr beam_hit_radius{200};
  impact_request const request{shot.muzzle.transform, shot.range, beam_hit_radius, projectile_payload(definition, shot.charge)};
  auto const distance{hit ? hit(request) : std::nullopt};
  // Original beam drawing uses the hit routine's depth-plus-one return directly.
  int32_t const length{distance ? wrap_long(static_cast<int64_t>(*distance) + 1) : shot.range};
  auto end{rotate_vector({0, length, 0}, shot.muzzle.transform)};
  for(unsigned int axis{0}; axis != 3; ++axis) end[axis] = wrap_long(static_cast<int64_t>(end[axis]) + shot.muzzle.transform.translation[axis]);
  return {vec3<int32_t>{shot.muzzle.transform.translation}, vec3<int32_t>{end}};
}

projectile_instance::projectile_instance(weapon_shot const &shot, weapon_catalogue const &definitions, projectile_motions const &motion_data, model_resource const &bullet_models, model_resource const &rocket_models, bool const target_present, int16_t const initial_rocket_clock)
  : source{definitions.projectiles.at(shot.projectile)}, motions{motion_data},
    geometry_source{projectile_shape(source, motions, bullet_models, rocket_models)}, shape{std::get<shape_part>(geometry_source.content)}, visual{shape}, damage_scale{shot.kind == shot_kind::scaled ? shot.charge : int16_t{0}} {
  flight_state.kind = source.kind;
  flight_state.subtype = static_cast<uint8_t>(source.subtype);
  flight_state.position = shot.muzzle.position;
  flight_state.angles = shot.muzzle.angles;
  flight_state.speed = source.kind == projectile_kind::rocket ? wrap_word(rocket_start_speed + shot.inherited_speed) : source.speed;
  flight_state.animation_remaining = source.kind == projectile_kind::rocket ? initial_rocket_clock : motions.bullets.at(flight_state.subtype).animation_period;
  flight_state.cells.resize(shape.cell_frame_counts.size(), 0);
  for(auto const count : shape.cell_frame_counts) cell_counts.push_back(static_cast<uint16_t>(count));
  flight_state.target_present = source.kind == projectile_kind::rocket && target_present;
}

projectile_flight const &projectile_instance::flight() const { return flight_state; }
model_instance const &projectile_instance::model() const { return visual; }
model_part const &projectile_instance::geometry() const { return geometry_source; }
void projectile_instance::clear_target() { flight_state.target_present = false; }

model_transform projectile_instance::placement() const {
  auto result{rotation_transform(flight_state.angles)};
  result.translation = flight_state.position;
  return result;
}

projectile_result projectile_instance::advance(frame_ratio const ratio, projectile_guidance &guidance, impact_query const &hit) {
  auto const result{advance_projectile(flight_state, motions, source.speed, cell_counts, ratio, guidance, [&](model_transform const &transform, int16_t const reach, int16_t const radius){
    return hit && hit({transform, reach, radius, projectile_payload(source, damage_scale)}).has_value();
  })};
  std::ranges::copy(flight_state.cells, visual.cell_frames().begin());
  return result;
}

} // namespace earthsiege
