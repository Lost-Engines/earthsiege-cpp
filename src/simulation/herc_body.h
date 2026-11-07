#pragma once
#include <functional>
#include <optional>
#include "resources/herc_definition.h"
#include "simulation/actor_contact.h"
#include "simulation/aim_control.h"
#include "simulation/animation_rig.h"
#include "simulation/locomotion.h"
#include "simulation/footsteps.h"
#include "simulation/sound_request.h"
#include "simulation/weapon_assembly.h"

namespace earthsiege {

struct damage_consequences;
struct damage_state;

struct herc_controls {
  int16_t throttle{0};
  int16_t steering{0};
  int16_t torso{0};
  int16_t elevation{0};
};

/// Own one HERC's movement, aiming, articulated state and mounted geometry
class herc_body {
  herc_definition const &source;
  sound_sink sounds;
  animation_rig rig;
  weapon_assembly armament;
  locomotion_state movement_state;
  movement_damage damage;
  bool throttle_command_pending{false};
  bool expansion{false};
  int16_t direction_mode{};
  std::vector<movement_event> frame_movement_events;
  aiming_state torso_state;
  aiming_state elevation_state;
  footstep_state footsteps;
  std::array<herc_ground_attachment,5> ground;
  void update_ground_attachments();
  size_t body_channel{0};
  std::optional<size_t> torso_channel;
  std::optional<size_t> elevation_channel;

public:
  herc_body(herc_definition const &definition, weapon_catalogue const &weapons, model_resource const &templates, std::span<int16_t const> equipment, vec3<int32_t> const &position, vec3<uint16_t> const &angles, sound_sink sounds = {});
  void place(vec3<int32_t> const &position, vec3<uint16_t> const &angles);
  void mark_collapsed();
  void apply_damage(damage_consequences const &consequences);
  void apply_damage(damage_state const &state);
  void update_weapons(weapon_bank const &bank);
  void recover_from_collision(int32_t duration, bool reverse);
  std::span<movement_event const> movement_events() const;
  // Inputs/controllers prepare channels separately from the world's advance phase.
  void begin_frame();
  void prepare_player(herc_controls const &controls, frame_ratio ratio, movement_event_sink const &events = {});
  void prepare_player_movement(int16_t throttle, int16_t steering, frame_ratio ratio, movement_event_sink const &events = {});
  void reset_aim_angles();
  /// Original 13250: preserve the player cockpit heading when the legs fail.
  void align_disabled_player();
  void stop_throttle();
  void movement_direction(int16_t mode){direction_mode=mode;}
  /// Keyboard/stop commands take precedence over the cockpit slider until exchanged.
  int16_t exchange_throttle(int16_t requested);
  void prepare_requested(int16_t speed, int16_t steering, frame_ratio ratio, movement_event_sink const &events = {}, movement_adjustment const &avoidance = {});
  void prepare_aim(vec2<int16_t> const &input, frame_ratio ratio, std::optional<vec2<int16_t>> target = {});
  bool advance(frame_ratio ratio, std::function<bool(herc_body const &)> const &contact = {}, std::function<void()> const &blocked = {});
  std::vector<uint16_t> footstep_sounds();
  auto &ground_attachments(){return ground;}
  model_transform aiming_pivot() const;
  /// Advance player controls; return whether either contact probe blocked motion
  bool step(herc_controls const &controls, int16_t time_ratio, std::function<bool(herc_body const &)> const &contact = {}, movement_event_sink const &events = {});
  bool step_requested(int16_t speed, int16_t steering, vec2<int16_t> const &aim, int16_t time_ratio, std::function<bool(herc_body const &)> const &contact = {}, movement_event_sink const &events = {});
  locomotion_state const &movement() const;
  aiming_state const &torso() const;
  aiming_state const &elevation() const;
  model_instance const &model() const;
  weapon_assembly const &weapons() const;
  model_transform placement() const;
  contact_actor contact_geometry() const;
};

} // namespace earthsiege
