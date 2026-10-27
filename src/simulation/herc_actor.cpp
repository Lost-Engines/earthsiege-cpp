#include "herc_actor.h"

namespace earthsiege {

herc_actor::herc_actor(herc_definition const &definition, weapon_catalogue const &weapons, model_resource const &templates, std::span<int16_t const> const equipment, vec3<int32_t> const &position, vec3<uint16_t> const &angles)
  : source{definition}, loadout{assemble_herc_loadout(definition, equipment, weapons)},
    body_state{definition, weapons, templates, equipment, position, angles},
    components{loadout.damage, initial_herc_damage(loadout, body_state.model().cell_frames().size())},
    power_state{.reserve{herc_power_capacity}, .shields{static_cast<int16_t>(definition.profile.shield_capacity >> 1), static_cast<int16_t>(definition.profile.shield_capacity >> 1), shield_balance_one / 2, definition.profile.shield_capacity}} {}

herc_body const &herc_actor::body() const { return body_state; }
earthsiege::damage_state const &herc_actor::damage() const { return components.state(); }
herc_power const &herc_actor::power() const { return power_state; }
herc_loadout const &herc_actor::definition() const { return loadout; }

std::vector<size_t> herc_actor::damage_section(size_t const section, int16_t const amount, component_choice const &choose, section_detachment const &detach) {
  auto removed{components.hit(section, amount, source.profile.legs, choose, detach)};
  body_state.apply_damage(components.state());
  return removed;
}

std::optional<component_hit> herc_actor::component_hit_test(model_transform const &actor_to_ray, int16_t const reach, int16_t const radius) const {
  return intersect_components(loadout.collision, components.state().present, source.models.roots.front(), body_state.model().transforms(), actor_to_ray, reach, radius);
}

bool herc_actor::step(herc_controls const &controls, frame_ratio const ratio, std::function<bool(herc_body const &)> const &contact, movement_event_sink const &events) {
  return body_state.step(controls, ratio, contact, events);
}

void herc_actor::recover_from_collision(int32_t const duration, bool const reverse) { body_state.recover_from_collision(duration, reverse); }

} // namespace earthsiege
