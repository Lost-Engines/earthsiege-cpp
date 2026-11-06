#pragma once
#include <memory>
#include <variant>
#include "render/scenery_renderer.h"
#include "resources/ground_definition.h"
#include "simulation/animation_rig.h"
#include "simulation/herc_impact.h"
#include "simulation/projectile_instance.h"

namespace earthsiege {

struct turret_aim_state {
  std::array<int16_t, 2> rates{};
  std::array<int16_t, 2> angles{};
};

struct turret_target {
  vec3<int32_t> position;
  vec3<int16_t> offset;
  vec3<uint16_t> angles;
  int16_t speed{0};
};

struct turret_fire_state {
  std::optional<size_t> target;
  int16_t target_timer{0};
  int16_t burst_timer{0};
  int16_t refire_timer{0};
  bool burst_phase{false};
};

struct turret_callbacks {
  std::function<std::optional<size_t>()> select;
  std::function<turret_target(size_t)> target;
  std::function<void(weapon_shot const &)> fire;
};

struct landing_weapon_state {int16_t refire{},phase_timer{};bool phase{};};
struct landing_bank_state {std::array<landing_weapon_state,3> weapons;int16_t target_timer{};std::optional<size_t> target;};
struct landing_callbacks {
  std::function<std::optional<size_t>(uint16_t)> select;
  std::function<turret_target(size_t)> target;
  std::function<void(weapon_shot const &,size_t)> fire;
};
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
  vec3<uint16_t> original_angles;
  int16_t accumulated_damage{0};
  std::array<int16_t,8> section_damage{},section_present{1,1,1,1,1,1,1,1};
  std::array<landing_bank_state,3> landing_banks_;
  bool destroyed{false};
  bool wreck_ready{false};
  int16_t destruction_steps{0};
  int16_t destruction_timer{0};
  int16_t cell_timer{0};
  turret_aim_state aiming;
  turret_fire_state firing;
  void advance_turret_cells(frame_ratio ratio);
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
  bool is_detectable() const;
  bool is_turret() const;
  bool is_landing_craft()const{return type==10;}
  auto const &landing_banks()const{return landing_banks_;}
  auto const &sections()const{return section_damage;}
  auto const &present_sections()const{return section_present;}
  vec3<int32_t> target_position() const;
  bool is_wreck() const;
  int16_t remaining_bursts() const;
  int16_t burst_timer() const;
  int16_t cell_clock() const;
  void direct_damage(int16_t amount, random_state &random, ground_damage_events const &events);
  void section_hit(size_t section,int16_t amount,random_state &random,ground_damage_events const &events);
  void advance_landing_craft(frame_ratio ratio,weapon_catalogue const &weapons,random_state &random,landing_callbacks const &callbacks);
  std::optional<int32_t> receive_impact(impact_request const &request, impact_conditions const &conditions, random_state &random, ground_damage_events const &events);
  void advance(frame_ratio ratio, random_state &random, ground_damage_events const &events);
  turret_aim_state const &turret_aim() const;
  /// Return the pre-turn local pitch/yaw error, as used by the firing gate.
  vec3<int16_t> track_point(vec3<int32_t> const &position);
  turret_fire_state const &turret_fire() const;
  void advance_turret(frame_ratio ratio, weapon_catalogue const &weapons, turret_callbacks const &callbacks);
  model_instance const &instance() const;
  model_transform const &placement() const;
  scene_model model() const;
  /// Only static bases participate in the original ground-height occupancy pass.
  std::optional<scenery_collider> occupancy() const;
  std::optional<scenery_collider> obstruction() const;
};

} // namespace earthsiege
