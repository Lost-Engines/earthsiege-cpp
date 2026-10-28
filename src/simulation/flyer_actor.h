#pragma once
#include "render/scenery_renderer.h"
#include "resources/flyer_definition.h"
#include "simulation/component_damage.h"
#include "simulation/herc_impact.h"
#include "simulation/flyer_motion.h"
#include "simulation/projectile_instance.h"

namespace earthsiege {
struct flyer_damage_events {
  std::function<void(int16_t, vec3<int32_t> const &, random_state &)> explosion;
  std::function<void(int16_t, vec3<int32_t> const &, random_state &)> debris;
  std::function<void()> kill_credit;
  std::function<void()> destroyed;
  std::function<void()> locked_target_destroyed;
};

/// Skimmers have one armour/internal section, no shield and no persistent wreck.
class flyer_actor {
  flyer_definition const &definition;
  model_instance visual;
  component_damage components;
  flyer_motion flight;
  bool destroyed{false};
  bool detectable{false};

public:
  flyer_actor(flyer_definition const &definition, vec3<int32_t> const &position, vec3<uint16_t> const &angles);
  flyer_actor(flyer_actor const &) = delete;
  flyer_actor(flyer_actor &&) = delete;
  flyer_actor &operator=(flyer_actor const &) = delete;
  flyer_actor &operator=(flyer_actor &&) = delete;
  flyer_profile const &profile() const;
  model_instance const &instance() const;
  model_transform placement() const;
  flyer_motion &motion();
  flyer_motion const &motion() const;
  vec3<uint16_t> const &angles() const;
  vec3<int16_t> const &local_velocity() const;
  vec3<int16_t> const &angular_velocity() const;
  damage_state const &damage() const;
  bool dead() const;
  bool is_detectable() const;
  void set_detectable(bool value);
  bool direct_damage(int16_t amount, random_state &random, flyer_damage_events const &events, bool player_locked_target = false);
  std::optional<int32_t> receive_impact(impact_request const &request, impact_conditions const &conditions, random_state &random, flyer_damage_events const &events, bool player_locked_target = false);
  scene_model model() const;
};
} // namespace earthsiege
