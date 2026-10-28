#include "herc_actor.h"
#include <array>
#include <stdexcept>
#include <utility>
#include "resources/model_tree.h"
#include "simulation/fixed_math.h"
#include "simulation/impact_damage.h"
#include "simulation/model_math.h"

namespace earthsiege {

herc_actor::herc_actor(herc_definition const &definition, weapon_catalogue const &weapons, model_resource const &templates, std::span<int16_t const> const equipment, vec3<int32_t> const &position, vec3<uint16_t> const &angles, std::span<int16_t const> const weapon_values)
  : source{definition}, weapon_definitions{weapons}, loadout{assemble_herc_loadout(definition, equipment, weapons)},
    body_state{definition, weapons, templates, equipment, position, angles},
    components{loadout.damage, initial_herc_damage(loadout, body_state.model().cell_frames().size())},
    power_state{.reserve{herc_power_capacity}, .shields{static_cast<int16_t>(definition.profile.shield_capacity >> 1), static_cast<int16_t>(definition.profile.shield_capacity >> 1), shield_balance_one / 2, definition.profile.shield_capacity}},
    armament{weapons, templates, definition.mounts, equipment, weapon_values},
    salvage_excluded{definition.profile.salvage_value == 0} {}

herc_body const &herc_actor::body() const { return body_state; }
void herc_actor::place(vec3<int32_t> const &position, vec3<uint16_t> const &angles) { body_state.place(position, angles); }
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
  auto shield_damage{adjust_impact_damage(request.damage.shield, conditions)};
  auto armour_damage{adjust_impact_damage(request.damage.armour, conditions)};
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

std::pair<int16_t, vec3<int16_t>> herc_actor::section_centre(size_t const section, bool const last) const {
  std::optional<std::pair<int16_t, vec3<int16_t>>> centre;
  for(auto const &part : loadout.collision) for(auto const &entry : part.sections) {
    if(entry.section == static_cast<int16_t>(section)) {
      centre.emplace(part.model_part, collision_bounds(entry.spheres).centre);
      if(!last) return *centre;
    }
  }
  if(centre) return *centre;
  throw std::out_of_range{"damage section has no collision centre"};
}

vec3<int32_t> herc_actor::section_position(size_t const section, bool const radial) const {
  auto const [part, centre]{section_centre(section, radial)};
  // Radial damage's cached lookup tests > 0; detachment also transforms part zero.
  if(radial ? part <= 0 : part < 0) return transform_point(centre, body_state.placement());
  auto const node{find_model_part(source.models.roots.front(), part)};
  if(!node || node->header.transform < 0) throw std::out_of_range{"damage section model transform"};
  auto const local{transform_point(centre, body_state.model().transforms()[static_cast<size_t>(node->header.transform)])};
  auto result{rotate_vector(local, body_state.placement())};
  for(unsigned int axis{0}; axis != 3; ++axis) result[axis] = wrap_long(static_cast<int64_t>(result[axis]) + body_state.placement().translation[axis]);
  return result;
}

vec3<int32_t> herc_actor::target_position(int16_t const component) const {
  if(component >= 0) return section_position(static_cast<size_t>(component), true);
  auto const node{find_model_part(source.models.roots.front(), source.profile.camera_part)};
  if(!node) return vec3<int32_t>{body_state.movement().position};
  auto const local{body_state.model().transforms()[static_cast<size_t>(node->header.transform)].translation};
  auto position{rotate_vector(local, body_state.placement())};
  for(unsigned int axis{0}; axis != 3; ++axis) position[axis] = wrap_long(static_cast<int64_t>(position[axis]) + body_state.movement().position[axis]);
  return position;
}

int16_t herc_actor::target_component(vec3<int32_t> const &origin, vec3<uint16_t> const &angles) const {
  int32_t best{0x1000000};
  int16_t selected{-1};
  auto const magnitude{[](int16_t value){ return value == -32768 ? 32767 : std::abs(static_cast<int>(value)); }};
  // The original subtracts all three angles, but ranks pitch and roll only.
  // Yaw does not participate; ties retain the first present damage section.
  for(size_t i{0}; i != components.state().present.size(); ++i) {
    if(!components.state().present[i]) continue;
    auto const point{target_position(static_cast<int16_t>(i))};
    vec3<int32_t> delta;
    for(unsigned int axis{0}; axis != 3; ++axis) delta[axis] = wrap_long(static_cast<int64_t>(point[axis]) - origin[axis]);
    auto const pitch{direction_angle(fast_distance_2d({delta.x, delta.y}), delta.z)};
    auto const score{magnitude(wrap_word(pitch - angles.x)) + magnitude(wrap_word(-angles.y))};
    if(score < best) { best = score; selected = static_cast<int16_t>(i); }
  }
  return selected;
}

void herc_actor::detach_section(size_t const section, bool &large_explosion, random_state &random, herc_damage_events const &events) {
  auto const &piece{loadout.damage.components.at(section)};
  auto const flag{[&](damage_section_flag const value){ return (piece.flags & std::to_underlying(value)) != 0; }};
  if(!large_explosion && piece.cell_sequence >= 0 && flag(damage_section_flag::attached_effect) && events.attached_smoke) {
    auto const [part, centre]{section_centre(section)};
    events.attached_smoke(part, centre);
  }
  auto const position{section_position(section, false)};
  if(!large_explosion) {
    large_explosion = flag(damage_section_flag::large_explosion);
    events.explosion(large_explosion ? 17 : 10, position, random);
  }
  events.debris(3, position, random);
}

bool herc_actor::allows_salvage() const { return !salvage_excluded; }

void herc_actor::restore_condition(herc_condition const &condition) { components.restore_condition(condition); }

void herc_actor::direct_damage(size_t const section, int16_t const amount, random_state &random, herc_damage_events const &events) {
  if(!events.explosion || !events.debris) throw std::invalid_argument{"damage requires synchronous effect emitters"};
  if(events.body_damage) events.body_damage(section, amount);
  bool large_explosion{false};
  damage_section(section, amount, [&](int16_t const bound){ return random.bounded(bound); }, [&](size_t const detached){ detach_section(detached, large_explosion, random, events); });
}

void herc_actor::random_damage(int16_t const chance, int16_t const magnitude, random_state &random, herc_damage_events const &events) {
  if(magnitude == 0) return;
  for(size_t section{0}; section != components.state().present.size(); ++section) {
    if(!components.state().present[section] || (random.next() & 255) >= chance) continue;
    auto const multiplier{random.bounded(magnitude) + (magnitude >> 1)};
    auto const amount{wrap_word((components.section_capacity(section) * multiplier + 128) >> 8)};
    direct_damage(section, amount, random, events);
  }
}

void herc_actor::apply_starting_condition(int16_t const condition, random_state &random, herc_damage_events const &events) {
  int16_t magnitude{0};
  if(condition >= 0 && condition < 80) {
    if(condition < 20) {
      direct_damage((random.next() & 1) ? 7 : 8, 32000, random, events);
      if(source.profile.legs > 2) direct_damage((random.next() & 1) ? 13 : 14, 32000, random, events);
      magnitude = 150;
      body_state.mark_collapsed();
    } else if(condition < 40) magnitude = 120;
    else if(condition < 60) magnitude = 80;
    else magnitude = 50;
    components.apply_starting_impairments(condition);
    if(condition < 40) salvage_excluded = true;
  }
  random_damage(static_cast<int16_t>(magnitude + 25), magnitude, random, events);
  body_state.apply_damage(components.state());
}

std::optional<herc_interception> herc_actor::receive_impact(impact_request const &request, impact_conditions const &conditions, random_state &random, herc_damage_events const &events) {
  if(!events.explosion || !events.debris) throw std::invalid_argument{"impact requires synchronous effect emitters"};
  auto result{intercept(request, conditions, events.shield_warning)};
  if(!result) return result;
  auto const choose{[&](int16_t const bound){ return random.bounded(bound); }};
  int constexpr probability_mask{4095};                                        // original Q12 chance draws
  int constexpr radial_damage_chance{2050};
  int constexpr probability_percent{41};
  bool large_explosion{false};
  auto const detach{[&](size_t const section){
    detach_section(section, large_explosion, random, events);
  }};
  auto const damage{[&](size_t const section, int16_t const amount){
    if(events.body_damage) events.body_damage(section, amount);
    if(conditions.player && conditions.invulnerable && !conditions.difficulty_override) return;
    large_explosion = false;                                                   // suppression lasts only for this section/dependent cascade
    damage_section(section, amount, choose, detach);
  }};
  auto effects{&request.damage.shield_effects};
  int16_t debris_group{-1};
  bool destroyed{false};
  if(result->body) {
    auto const &body{*result->body};
    auto const previous{components.fraction(body.section) >> 5};
    damage(body.section, body.direct);
    if(body.radial != 0) {
      int32_t constexpr radius{500};
      for(size_t section{0}; section != components.state().present.size(); ++section) {
        if(!components.state().present[section] || (random.next() & probability_mask) >= radial_damage_chance) continue;
        auto const position{section_position(section, true)};
        vec3<int32_t> delta;
        for(unsigned int axis{0}; axis != 3; ++axis) delta[axis] = wrap_long(static_cast<int64_t>(result->position[axis]) - position[axis]);
        auto const distance{fast_distance(delta)};
        if(distance < radius) damage(section, wrap_word(body.radial * (radius - distance) / radius));
      }
    }
    auto const fraction{components.fraction(body.section)};
    destroyed = fraction == 256;
    if((fraction >> 5) == previous) {
      effects = &request.damage.ground_effects;
      debris_group = 1;
    } else {
      effects = &request.damage.armour_effects;
      debris_group = 2;
      if(body.section < 2 && conditions.player && events.cockpit_damage) events.cockpit_damage(body.section);
      if(!destroyed && source.profile.weapons_can_detach && body.section >= weapon_section_first) {
        int const threshold{(conditions.side == 0 ? 3 : 10) * probability_percent};
        if((random.next() & probability_mask) < threshold) {
          auto const equipment_slot{body.section - weapon_section_first};
          for(size_t slot{0}; slot != armament.slots().size(); ++slot) {
            if(!armament.slots()[slot] || armament.slots()[slot]->equipment_slot != static_cast<int8_t>(equipment_slot)) continue;
            auto &weapon{armament.at(slot)};
            if(!weapon.controls().disabled) {
              weapon.controls().disabled = true;
              if(source.mounts.at(slot).external() && events.weapon_ejected) events.weapon_ejected(slot);
            }
            if(conditions.side == 1 && events.salvage) {
              auto const condition{wrap_word((500 * fraction + 512) >> 10)};
              events.salvage(weapon.definition, wrap_word((wrap_word(256 - condition) * 100) >> 8));
            }
            large_explosion = false;
            components.knock_off_weapon(body.section, choose, detach);
            armament.disable_missing_sections(components.state().present);
            body_state.apply_damage(components.state());
            break;
          }
        }
      }
    }
  }
  if(!destroyed) {
    events.explosion(effects->at(static_cast<size_t>(random.next() & 3)), result->position, random);
    if(result->body) events.debris(debris_group, result->position, random);
  }
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
  bool const was_dead{components.state().consequences.dead};
  auto removed{components.hit(section, amount, source.profile.legs, choose, detach)};
  armament.disable_missing_sections(components.state().present);
  body_state.apply_damage(components.state());
  if(!was_dead && components.state().consequences.dead) impact_state.detectable = false;
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
