#include "herc_actor.h"
#include <array>
#include <stdexcept>
#include "simulation/fixed_math.h"
#include "simulation/model_math.h"

namespace earthsiege {

herc_actor::herc_actor(herc_definition const &definition, weapon_catalogue const &weapons, model_resource const &templates, std::span<int16_t const> const equipment, vec3<int32_t> const &position, vec3<uint16_t> const &angles, std::span<int16_t const> const weapon_values)
  : source{definition}, weapon_definitions{weapons}, loadout{assemble_herc_loadout(definition, equipment, weapons)},
    body_state{definition, weapons, templates, equipment, position, angles},
    components{loadout.damage, initial_herc_damage(loadout, body_state.model().cell_frames().size())},
    power_state{.reserve{herc_power_capacity}, .shields{static_cast<int16_t>(definition.profile.shield_capacity >> 1), static_cast<int16_t>(definition.profile.shield_capacity >> 1), shield_balance_one / 2, definition.profile.shield_capacity}},
    armament{weapons, templates, definition.mounts, equipment, weapon_values} {}

herc_body const &herc_actor::body() const { return body_state; }
earthsiege::damage_state const &herc_actor::damage() const { return components.state(); }
herc_power const &herc_actor::power() const { return power_state; }
herc_power &herc_actor::power() { return power_state; }
herc_impact_state const &herc_actor::impacts() const { return impact_state; }
herc_impact_state &herc_actor::impacts() { return impact_state; }
weapon_bank &herc_actor::weapons() { return armament; }
weapon_bank const &herc_actor::weapons() const { return armament; }

std::optional<herc_interception> herc_actor::intercept(impact_request const &request, impact_conditions const &conditions, std::function<void()> const &shield_warning) {
  auto const placement{body_state.placement()};
  vec3<int32_t> delta;
  for(unsigned int axis{0}; axis != 3; ++axis) delta[axis] = wrap_long(static_cast<int64_t>(placement.translation[axis]) - request.transform.translation[axis]);
  auto const maximum{wrap_long(static_cast<int64_t>(request.radius) + request.reach + source.profile.shield_radius)};
  if(fast_distance(delta) > maximum) return std::nullopt;
  auto shield_damage{request.damage.shield};
  auto armour_damage{request.damage.armour};
  if(conditions.attacker_side == 0) {
    std::array<int16_t, 4> constexpr multipliers{2800, 2000, 1400, 1000};
    auto const multiplier{multipliers.at(conditions.difficulty_override ? 3 : static_cast<size_t>(conditions.difficulty))};
    shield_damage = wrap_word((shield_damage * multiplier + 512) >> 10);
    armour_damage = wrap_word((armour_damage * multiplier + 512) >> 10);
  }
  if(!conditions.player && request.damage.missile_subtype) {
    auto const signature{*request.damage.missile_subtype};
    if((signature == 0 || signature == 1) && impact_state.ecm_remaining == 0) impact_state.detectable = true;
    else if(signature == 2) {
      impact_state.detectable = false;
      int16_t constexpr ecm_duration{6000};
      impact_state.ecm_remaining = ecm_duration;
    }
  }
  auto const actor_to_ray{compose_transforms(placement, inverse_transform(request.transform))};
  auto remaining{shield_damage};
  auto const shield{intercept_shield(power_state.shields, remaining, source.profile.shield_height, source.profile.shield_radius, request.reach, actor_to_ray)};
  if(!shield) return std::nullopt;
  impact_state.damage_received = wrap_long(static_cast<int64_t>(impact_state.damage_received) + shield_damage - remaining);
  herc_interception result;
  result.distance = wrap_long(static_cast<int64_t>(*shield) + 1);
  if(remaining != 0) {
    if(conditions.player && !impact_state.shield_warning) {
      if(shield_warning) shield_warning();
      impact_state.shield_warning = true;
    }
    auto const component{component_hit_test(actor_to_ray, request.reach, request.radius)};
    if(!component) return std::nullopt;
    result.distance = wrap_long(static_cast<int64_t>(component->distance) + 1);
    impact_state.damage_received = wrap_long(static_cast<int64_t>(impact_state.damage_received) + armour_damage);
    auto const radial{wrap_word((armour_damage * request.damage.splash_q10 + 512) >> 10)};
    result.body = body_damage_request{static_cast<size_t>(component->section), wrap_word(armour_damage - radial), radial};
  }
  result.position = rotate_vector({0, result.distance, 0}, request.transform);
  for(unsigned int axis{0}; axis != 3; ++axis) result.position[axis] = wrap_long(static_cast<int64_t>(result.position[axis]) + request.transform.translation[axis]);
  return result;
}

void herc_actor::update_power(frame_ratio const ratio, bool const free_energy) {
  update_herc_power(power_state, components.state().consequences, ratio, [&](int16_t const budget){ return armament.allocate(-1, budget, ratio, free_energy); });
  body_state.update_weapons(armament);
}

std::vector<weapon_shot> herc_actor::discharge_slot(size_t const slot, vec2<int16_t> const &aim, bool const free_ammunition) {
  auto &weapon{armament.at(slot)};
  auto const &mount{source.mounts.at(slot)};
  auto const muzzle{prepare_weapon_muzzle(mount, weapon_definitions.weapons.at(static_cast<size_t>(weapon.definition)), source.models.roots.front(), body_state.model().transforms(), body_state.placement(), aim)};
  auto shots{discharge_weapon(weapon, weapon_definitions, mount, muzzle, free_ammunition)};
  body_state.update_weapons(armament);
  return shots;
}

herc_loadout const &herc_actor::definition() const { return loadout; }

std::vector<size_t> herc_actor::damage_section(size_t const section, int16_t const amount, component_choice const &choose, section_detachment const &detach) {
  auto removed{components.hit(section, amount, source.profile.legs, choose, detach)};
  armament.disable_missing_sections(components.state().present);
  body_state.apply_damage(components.state());
  return removed;
}

std::optional<component_hit> herc_actor::component_hit_test(model_transform const &actor_to_ray, int32_t const reach, int16_t const radius) const {
  return intersect_components(loadout.collision, components.state().present, source.models.roots.front(), body_state.model().transforms(), actor_to_ray, reach, radius);
}

bool herc_actor::step(herc_controls const &controls, frame_ratio const ratio, std::function<bool(herc_body const &)> const &contact, movement_event_sink const &events) {
  return body_state.step(controls, ratio, contact, events);
}

void herc_actor::recover_from_collision(int32_t const duration, bool const reverse) { body_state.recover_from_collision(duration, reverse); }

} // namespace earthsiege
