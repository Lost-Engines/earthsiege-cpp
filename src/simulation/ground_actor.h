#pragma once
#include <memory>
#include <variant>
#include "render/scenery_renderer.h"
#include "resources/ground_definition.h"
#include "simulation/animation_rig.h"
#include "simulation/herc_impact.h"
#include "simulation/projectile_instance.h"

namespace earthsiege {

struct ground_damage_events {
  std::function<void(int16_t, vec3<int32_t> const &, random_state &)> explosion;
  std::function<void(int16_t, vec3<int32_t> const &, random_state &)> debris;
  std::function<void()> kill_credit;
  std::function<void()> destroyed;
  std::function<void(vec3<int16_t> const &)> attached_smoke;
};

/// Fixed mission actor; authored artwork, condition and animation state have one owner.
class ground_actor {
  ground_catalogue const &catalogue;
  size_t type;
  uint8_t side;
  std::variant<model_instance, animation_rig> intact;
  model_instance wreck;
  model_transform transform;
  int16_t accumulated_damage{0};
  bool destroyed{false};
  bool wreck_ready{false};
  int16_t destruction_steps{0};
  int16_t destruction_timer{0};
  int16_t cell_timer{0};
  model_instance &intact_instance();
  scenery_collider heightfield() const;
  void advance_cell();

public:
  ground_actor(ground_catalogue const &catalogue, size_t type, uint8_t side, int16_t condition, vec3<int32_t> const &position, vec3<uint16_t> const &angles);
  ground_actor(ground_actor const &) = delete;
  ground_actor(ground_actor &&) = delete;
  ground_actor &operator=(ground_actor const &) = delete;
  ground_actor &operator=(ground_actor &&) = delete;
  ground_profile const &profile() const;
  int16_t damage() const;
  bool dead() const;
  bool is_wreck() const;
  int16_t remaining_bursts() const;
  int16_t burst_timer() const;
  int16_t cell_clock() const;
  void direct_damage(int16_t amount, random_state &random, ground_damage_events const &events);
  std::optional<int32_t> receive_impact(impact_request const &request, impact_conditions const &conditions, random_state &random, ground_damage_events const &events);
  void advance(frame_ratio ratio, random_state &random, ground_damage_events const &events);
  model_instance const &instance() const;
  model_transform const &placement() const;
  scene_model model() const;
  /// Only static bases participate in the original ground-height occupancy pass.
  std::optional<scenery_collider> occupancy() const;
};

} // namespace earthsiege
