#pragma once
#include <functional>
#include "resources/herc_definition.h"
#include "simulation/actor_contact.h"
#include "simulation/aim_control.h"
#include "simulation/animation_rig.h"
#include "simulation/locomotion.h"
#include "simulation/weapon_assembly.h"

namespace earthsiege {

struct herc_controls {
  int16_t throttle{0};
  int16_t steering{0};
  int16_t torso{0};
  int16_t elevation{0};
};

/// Own one HERC's normal movement, aiming, articulated state and mounted geometry
class herc_body {
  herc_definition const &source;
  animation_rig rig;
  weapon_assembly armament;
  locomotion_state movement_state;
  aiming_state torso_state;
  aiming_state elevation_state;
  size_t body_channel{0};
  size_t torso_channel{0};
  size_t elevation_channel{0};
  bool advance_body(vec2<int16_t> const &aim, int16_t frame_ratio, std::function<bool(herc_body const &)> const &contact);

public:
  herc_body(herc_definition const &definition, weapon_catalogue const &weapons, model_resource const &templates, std::span<int16_t const> equipment, vec3<int32_t> const &position, vec3<uint16_t> const &angles);
  /// Advance normal-player controls; return whether either contact probe blocked motion
  bool step(herc_controls const &controls, int16_t frame_ratio, std::function<bool(herc_body const &)> const &contact = {});
  bool step_requested(int16_t speed, int16_t steering, vec2<int16_t> const &aim, int16_t frame_ratio, std::function<bool(herc_body const &)> const &contact = {});
  locomotion_state const &movement() const;
  aiming_state const &torso() const;
  aiming_state const &elevation() const;
  model_instance const &model() const;
  weapon_assembly const &weapons() const;
  model_transform placement() const;
  contact_actor contact_geometry() const;
};

} // namespace earthsiege
