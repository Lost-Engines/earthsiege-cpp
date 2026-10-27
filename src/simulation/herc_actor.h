#pragma once
#include "simulation/herc_body.h"
#include "simulation/herc_loadout.h"
#include "simulation/herc_power.h"
#include "simulation/hit_volumes.h"

namespace earthsiege {

/// Own one articulated HERC, its loadout-dependent hit volumes and changing combat state
class herc_actor {
  herc_definition const &source;
  herc_loadout loadout;
  herc_body body_state;
  component_damage components;
  herc_power power_state;

public:
  herc_actor(herc_definition const &definition, weapon_catalogue const &weapons, model_resource const &templates, std::span<int16_t const> equipment, vec3<int32_t> const &position, vec3<uint16_t> const &angles);
  herc_actor(herc_actor const &) = delete;
  herc_actor(herc_actor &&) = delete;
  herc_actor &operator=(herc_actor const &) = delete;
  herc_actor &operator=(herc_actor &&) = delete;
  herc_body const &body() const;
  damage_state const &damage() const;
  herc_power const &power() const;
  herc_loadout const &definition() const;
  /// Direct armour/component damage; projectile shielding and impact effects are separate stages
  std::vector<size_t> damage_section(size_t section, int16_t amount, component_choice const &choose, section_detachment const &detach = {});
  std::optional<component_hit> component_hit_test(model_transform const &actor_to_ray, int16_t reach, int16_t radius) const;
  bool step(herc_controls const &controls, frame_ratio ratio, std::function<bool(herc_body const &)> const &contact = {}, movement_event_sink const &events = {});
  void recover_from_collision(int32_t duration, bool reverse);
};

} // namespace earthsiege
