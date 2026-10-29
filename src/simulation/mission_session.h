#pragma once
#include <map>
#include "audio/combat_music.h"
#include <memory>
#include <vector>
#include "render/scenery_renderer.h"
#include "render/target_instruments.h"
#include "render/waypoint_instruments.h"
#include "render/damage_display.h"
#include "render/scanner_display.h"
#include "render/tactical_map.h"
#include "render/full_damage_display.h"
#include "resources/mission_script.h"
#include "resources/player_roster.h"
#include "resources/mission_results.h"
#include "simulation/camera.h"
#include "simulation/mission_activity.h"
#include "simulation/wingmate_orders.h"
#include "simulation/mission_objectives.h"
#include "simulation/flyer_attack.h"
#include "simulation/herc_actor.h"
#include "simulation/player_armament.h"
#include "simulation/player_input.h"
#include "simulation/herc_orders.h"
#include "simulation/herc_sensors.h"
#include "simulation/herc_reactions.h"
#include "simulation/hit_dispatch.h"
#include "simulation/herc_fire_control.h"
#include "simulation/ground_actor.h"
#include "simulation/flyer_actor.h"
#include "simulation/scenery_world.h"
#include "simulation/world_effects.h"
#include "simulation/world_projectiles.h"

namespace earthsiege {

class resource_store;

enum class combat_notice_kind { attached_smoke, ejected_weapon, salvage, shield_warning, cockpit_damage, friendly_warning, under_attack, mission_event };
struct combat_notice {
  combat_notice_kind kind;
  size_t actor{0};
  int16_t part{0};
  int16_t value{0};
  vec3<int32_t> position;
};

struct wingmate_radio_response { size_t actor; uint16_t message; };

struct player_rocket_launch {
  uint64_t sequence{};
  std::optional<size_t> slot;
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
    std::optional<int16_t> pilot_slot;
    bool contact_received{false};
    bool skip_decision{false};
    bool obstructed{false};
    bool radar_active{false};
    bool ignore_hud_target{false};
    std::optional<impact_obstruction> obstruction{};
    player_objective_progress objectives{};
    bool route_checked{false};
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
    herc_reaction_state reactions{};
    std::array<int16_t,3> statistics{};
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
  void hit_reaction(size_t victim, size_t attacker, int16_t damage);
  void friendly_fire_warning(size_t index);
  vec3<int32_t> obstruction_position(size_t index) const;
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
  int16_t player_chassis_id{0};
  mission_outcome_clock outcome_clock;
  std::optional<mission_status> pending_mission_prompt;
  std::pair<std::vector<objective_actor>, std::vector<objective_group>> objective_facts() const;
  void advance_player_objectives(size_t index, frame_ratio ratio);
  std::vector<actor_contact> frame_contacts;
  random_state random;
  random_state visual_random;
  std::vector<sound_request> sound_requests;
  world_effects effects;
  world_projectiles projectiles;
  player_rocket_launch camera_rocket;
  std::vector<wingmate_radio_response> radio_responses;
  std::unique_ptr<player_armament> player_weapons;
  int32_t player_weapon_range{};
  player_input_router input_router;
  std::optional<size_t> displayed_target;
  void advance_frame(player_input &input, frame_ratio ratio, std::function<void()> const &prepare_player);
  std::vector<combat_notice> frame_notices;
  std::function<void()> cockpit_impact_handler;
  std::vector<std::pair<int16_t,int16_t>> recovered_weapons;
  void credit_defeat(size_t attacker,size_t victim);
  std::optional<int32_t> impact(projectile_owner const &owner, impact_request const &request);
  void fire_selected(bool pressed);
  void launch(size_t owner, weapon_shot const &shot);
  vec3<int32_t> target_position(size_t actor, int16_t component = -1) const;
  herc_damage_events damage_events(size_t actor);
  ground_damage_events base_damage_events(size_t actor);
  flyer_damage_events flyer_events(size_t actor);
  std::vector<scene_model> herc_models(bool roster_only, bool include_player = true);

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
  int16_t player_chassis() const;
  size_t player_id() const;
  std::optional<int16_t> pilot_identity(size_t actor) const;
  damage_display_state instrument_damage(std::optional<size_t> actor) const;
  scanner_display_state scanner_state(uint16_t heading, uint32_t ticks, int32_t range) const;
  tactical_map_state tactical_state(uint32_t ticks) const;
  full_damage_state full_damage(std::optional<size_t> actor) const;
  std::vector<size_t> wingmate_ids() const;
  void request_wingmate_status(size_t actor);
  bool camera_ready() const;
  bool eo_selected() const;
  player_rocket_launch const &latest_player_rocket() const;
  void step(herc_controls const &controls, int16_t time_ratio, bool fire = false);
  bool select_weapon(size_t slot);
  void cycle_weapon();
  void set_auto_tracking(bool enabled);
  void toggle_weapon_pair();
  player_input_actions step_player(player_input input, frame_ratio ratio, bool external_control = false, rectangle aiming_viewport = {0, 0, 320, 200});
  void update_player_interface();
  void initialise_weapon_instruments();
  void exchange_weapon_instruments(frame_ratio ratio);
  void weapon_group_control(unsigned int button);
  void weapon_control(unsigned int control_number, bool secondary);
  void weapon_charge_request(unsigned int control_number, int32_t value);
  int16_t exchange_player_instruments(int16_t requested_throttle, std::optional<bool> shield_to_front = {});
  void clear_player_input();
  bool player_command(uint16_t code);
  wingmate_order_state order_state(size_t actor) const;
  bool transmit_order(uint16_t command);
  bool transmit_order(wingmate_order const &order, std::optional<size_t> recipient);
  std::vector<wingmate_radio_response> take_radio_responses();
  std::vector<sound_request> take_sounds();
  music_contacts music_observations()const;
  random_state &interface_random();
  void on_cockpit_impact(std::function<void()> handler) { cockpit_impact_handler=std::move(handler); }
  void cycle_target();
  void set_displayed_target(std::optional<size_t> target);
  std::optional<size_t> target_selection() const;
  std::optional<hud_target_state> target_marker() const;
  std::optional<hud_waypoint_state> next_waypoint() const;
  player_armament const &player_weapon_controls() const;
  herc_sensor_state const &player_sensors() const;
  mission_status mission_state(bool requested = false) const;
  int16_t result_grade() const;
  mission_results results() const;
  std::optional<mission_status> take_mission_prompt();
  mission_outcome_clock const &mission_clock() const;
  player_objective_progress const &objective_progress() const;
  std::optional<size_t> weapon_selection() const;
  herc_actor const &player_actor() const;
  world_effects &visual_effects();
  world_projectiles const &ordnance() const;
  std::span<combat_notice const> combat_notices() const;
  std::vector<scene_object> visible_objects(bool include_player = true);
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
