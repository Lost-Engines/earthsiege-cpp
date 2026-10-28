#pragma once
#include <utility>
#include "simulation/herc_body.h"
#include "simulation/herc_aiming.h"
#include "simulation/herc_impact.h"
#include "simulation/herc_loadout.h"
#include "simulation/herc_power.h"
#include "simulation/hit_volumes.h"
#include "simulation/projectile_instance.h"
#include "simulation/weapon_bank.h"
#include "simulation/weapon_firing.h"

namespace earthsiege {

/// Own one articulated HERC, its loadout-dependent hit volumes and changing combat state
class herc_actor {
  herc_definition const &source;
  weapon_catalogue const &weapon_definitions;
  herc_loadout loadout;
  herc_body body_state;
  component_damage components;
  herc_power power_state;
  herc_impact_state impact_state;
  weapon_bank armament;
  bool salvage_excluded{false};
  int16_t cached_combat_rating{0};
  bool combat_rating_valid{false};
  std::function<void(damage_consequences const &, damage_consequences const &)> consequence_notification;
  void detach_section(size_t section, bool &large_explosion, random_state &random, herc_damage_events const &events);
  void apply_aim(herc_aim_request const &request, frame_ratio ratio);
  std::pair<int16_t, vec3<int16_t>> section_centre(size_t section, bool last = false) const;
  vec3<int32_t> section_position(size_t section, bool radial) const;

public:
  herc_actor(herc_definition const &definition, weapon_catalogue const &weapons, model_resource const &templates, std::span<int16_t const> equipment, vec3<int32_t> const &position, vec3<uint16_t> const &angles, std::span<int16_t const> weapon_values = {});
  herc_actor(herc_actor const &) = delete;
  herc_actor(herc_actor &&) = delete;
  herc_actor &operator=(herc_actor const &) = delete;
  herc_actor &operator=(herc_actor &&) = delete;
  herc_body const &body() const;
  herc_profile const &profile() const;
  int16_t combat_rating() const;
  int16_t total_damage_fraction() const;
  int16_t internal_damage_fraction(size_t component) const;
  int16_t choose_aimed_component(herc_actor const &target, random_state &random) const;
  void notify_damage_changes(std::function<void(damage_consequences const &, damage_consequences const &)> callback);
  void place(vec3<int32_t> const &position, vec3<uint16_t> const &angles);
  damage_state const &damage() const;
  herc_power const &power() const;
  herc_power &power();
  herc_impact_state const &impacts() const;
  herc_impact_state &impacts();
  /// Absorb shields and prepare a body-damage request; damage/effects execute after this boundary
  std::optional<herc_interception> intercept(impact_request const &request, impact_conditions const &conditions, std::function<void()> const &shield_warning = {});
  /// Complete shielding, component damage, detachment, impact effects and salvage requests
  std::optional<herc_interception> receive_impact(impact_request const &request, impact_conditions const &conditions, random_state &random, herc_damage_events const &events);
  weapon_bank &weapons();
  weapon_bank const &weapons() const;
  void update_power(frame_ratio ratio, bool free_energy = false);
  vec3<int16_t> aim_at(vec3<int32_t> const &point, frame_ratio ratio);
  void centre_aim(int32_t distance, frame_ratio ratio);
  std::vector<weapon_shot> discharge_aimed_slot(size_t slot, bool free_ammunition = false);
  /// Readiness and group/trigger policy are checked by the controller before discharge
  std::vector<weapon_shot> discharge_slot(size_t slot, vec2<int16_t> const &aim, bool free_ammunition = false);
  herc_loadout const &definition() const;
  bool allows_salvage() const;
  int16_t target_component(vec3<int32_t> const &source, vec3<uint16_t> const &angles) const;
  vec3<int32_t> target_position(int16_t component = -1) const;
  void restore_condition(herc_condition const &condition);
  void apply_starting_condition(int16_t condition, random_state &random, herc_damage_events const &events);
  void random_damage(int16_t chance, int16_t magnitude, random_state &random, herc_damage_events const &events);
  void radial_damage(vec3<int32_t> const &centre, int16_t radius, int16_t amount, random_state &random, herc_damage_events const &events, bool invulnerable = false);
  void direct_damage(size_t section, int16_t amount, random_state &random, herc_damage_events const &events);
  /// Direct armour/component damage; projectile shielding and impact effects are separate stages
  std::vector<size_t> damage_section(size_t section, int16_t amount, component_choice const &choose, section_detachment const &detach = {});
  std::optional<component_hit> component_hit_test(model_transform const &actor_to_ray, int32_t reach, int16_t radius) const;
  bool step(herc_controls const &controls, frame_ratio ratio, std::function<bool(herc_body const &)> const &contact = {}, movement_event_sink const &events = {});
  void begin_frame();
  void prepare_player(herc_controls const &controls, frame_ratio ratio, movement_event_sink const &events = {});
  void prepare_requested(int16_t speed, int16_t steering, frame_ratio ratio, movement_event_sink const &events = {}, movement_adjustment const &avoidance = {});
  bool advance(frame_ratio ratio, std::function<bool(herc_body const &)> const &contact = {}, std::function<void()> const &blocked = {});
  void recover_from_collision(int32_t duration, bool reverse);
};

} // namespace earthsiege
