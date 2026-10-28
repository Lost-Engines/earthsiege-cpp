#pragma once
#include <map>
#include <memory>
#include <vector>
#include "render/scenery_renderer.h"
#include "resources/mission_script.h"
#include "resources/player_roster.h"
#include "simulation/camera.h"
#include "simulation/mission_activity.h"
#include "simulation/flyer_attack.h"
#include "simulation/herc_actor.h"
#include "simulation/herc_orders.h"
#include "simulation/herc_sensors.h"
#include "simulation/herc_fire_control.h"
#include "simulation/ground_actor.h"
#include "simulation/flyer_actor.h"
#include "simulation/scenery_world.h"
#include "simulation/world_effects.h"
#include "simulation/world_projectiles.h"

namespace earthsiege {

class resource_store;

enum class combat_notice_kind { attached_smoke, ejected_weapon, salvage, shield_warning, cockpit_damage, friendly_warning };
struct combat_notice {
  combat_notice_kind kind;
  size_t actor{0};
  int16_t part{0};
  int16_t value{0};
  vec3<int32_t> position;
};

struct mission_herc_view {
  size_t source;
  size_t group;
  uint8_t side;
  bool waiting;
  bool player_roster;
  bool player;
  herc_actor const &actor;
};

struct mission_flyer_view {
  size_t source;
  size_t group;
  uint8_t side;
  bool waiting;
  flyer_actor const &actor;
};

struct mission_base_view {
  size_t source;
  size_t group;
  uint8_t side;
  bool waiting;
  ground_actor const &actor;
};

/// Own mission terrain, HERC combat and live projectiles/effects while controllers are integrated
class mission_session {
  struct crew_member {
    herc_definition const *definition;
    std::unique_ptr<herc_actor> actor;
    std::vector<model_attachment> attachments;
    size_t source{0};
    size_t group{0};
    uint8_t side{0};
    bool waiting{false};
    bool player_roster{false};
    bool contact_received{false};
    bool skip_decision{false};
    bool obstructed{false};
    bool radar_active{false};
    bool objective_event{false};
    int16_t requested_speed{0};
    int16_t collision_bias{0};
    int16_t aim_tolerance{0};
    int16_t stop_status{0};
    vec3<int32_t> command_position{};
    std::optional<size_t> designated{};
    std::optional<size_t> followed{};
    herc_attack_state attack{};
    herc_travel_state travel{};
    herc_flee_state flee{};
    herc_skirt_state skirt{};
    herc_ram_state ram{};
    herc_fire_state fire{};
    herc_sensor_state sensors{};
  };
  struct flyer_member {
    std::unique_ptr<flyer_actor> actor;
    flyer_attack_state attack;
    bool skip_decision{false};
    size_t source{0};
    size_t group{0};
    uint8_t side{0};
    bool waiting{false};
  };
  struct base_member {
    std::unique_ptr<ground_actor> actor;
    size_t source{0};
    size_t group{0};
    uint8_t side{0};
    bool waiting{false};
  };
  mission_header settings;
  placement_catalogue layout;
  std::unique_ptr<mission_activity> activity;
  std::vector<mission_route> routes;
  std::vector<vec3<int32_t>> points;
  scenery_world terrain;
  ground_catalogue ground_definitions;
  std::vector<base_member> bases;
  weapon_catalogue weapon_definitions;
  model_resource weapon_models;
  std::vector<bitmap> weapon_textures;
  std::map<int16_t, std::unique_ptr<herc_definition>> definitions;
  std::vector<crew_member> crew;
  std::map<int16_t, std::unique_ptr<flyer_definition>> flyer_definitions;
  std::vector<flyer_member> flyers;
  size_t flyer_id(size_t index) const;
  size_t base_id(size_t index) const;
  size_t actor_group(size_t index) const;
  std::vector<observed_actor> observe() const;
  int16_t actor_speed(size_t index) const;
  void navigate_flyer(size_t index);
  void dispatch_herc(size_t index, frame_ratio ratio);
  void enter_herc(size_t index, behavior type);
  void resume_herc_orders(size_t index);
  void reconsider_herc(size_t index);
  void navigate_herc(size_t index, frame_ratio ratio);
  void move_herc(size_t index, movement_request const &request, frame_ratio ratio);
  void aim_herc(size_t index, size_t target, int16_t facing, frame_ratio ratio);
  void advance_herc(size_t index, frame_ratio ratio, bool ramming = false);
  void update_herc_sensors(size_t index, frame_ratio ratio);
  bool contact_herc(size_t index);
  void damage_consequences_changed(size_t index, damage_consequences const &previous, damage_consequences const &current);
  void movement_event_received(size_t index, movement_event event);
  std::vector<contact_actor> contact_bodies() const;
  herc_combat_position combat_position(size_t index) const;
  vec3<int32_t> guard_anchor(size_t index) const;
  bool herc_target_completed(size_t index) const;
  bool enter_skirting(size_t index);
  bool target_path_blocked(size_t index) const;
  void dispatch_flyer(size_t index, frame_ratio ratio);
  void advance_base(size_t index, frame_ratio ratio);
  std::optional<int32_t> obstacle_distance(avoidance_probe kind, vec3<int32_t> const &start, vec3<int32_t> const &end) const;
  size_t player_index{0};
  int16_t mission_event_delay{0};
  std::vector<actor_contact> frame_contacts;
  random_state random;
  world_effects effects;
  world_projectiles projectiles;
  std::optional<size_t> selected_weapon;
  std::vector<combat_notice> frame_notices;
  std::optional<int32_t> impact(projectile_owner const &owner, impact_request const &request);
  void fire_selected(bool pressed);
  void launch(size_t owner, weapon_shot const &shot);
  vec3<int32_t> target_position(size_t actor, int16_t component = -1) const;
  herc_damage_events damage_events(size_t actor);
  ground_damage_events base_damage_events(size_t actor);
  flyer_damage_events flyer_events(size_t actor);
  std::vector<scene_model> herc_models(bool roster_only);

public:
  mission_session(resource_store const &resources, mission_script const &script, player_roster const &roster, random_state initial_random = random_state{}, mission_counters initial_counters = {});
  mission_session(mission_session const &) = delete;
  mission_session(mission_session &&) = delete;
  mission_session &operator=(mission_session const &) = delete;
  mission_session &operator=(mission_session &&) = delete;
  scenery_world const &scenery() const;
  mission_counters const &mission_variables() const;
  mission_activity const &activity_state() const;
  herc_body const &player() const;
  void step(herc_controls const &controls, int16_t time_ratio, bool fire = false);
  bool select_weapon(size_t slot);
  void cycle_weapon();
  std::optional<size_t> weapon_selection() const;
  herc_actor const &player_actor() const;
  world_effects &visual_effects();
  world_projectiles const &ordnance() const;
  std::span<combat_notice const> combat_notices() const;
  std::vector<scene_object> visible_objects();
  std::span<actor_contact const> contacts() const;
  std::vector<scene_model> roster_models();
  std::vector<mission_herc_view> hercs() const;
  std::vector<mission_base_view> ground_actors() const;
  std::vector<mission_flyer_view> flying_actors() const;
  random_state::state const &random_snapshot() const;
  camera_target view_target() const;
  camera_state make_camera(camera_mode mode) const;
};

} // namespace earthsiege
