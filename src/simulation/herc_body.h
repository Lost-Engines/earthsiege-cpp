#pragma once
#include "resources/herc_definition.h"
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

public:
  herc_body(herc_definition const &definition, weapon_catalogue const &weapons, model_resource const &templates, std::span<int16_t const> equipment, vec3<int32_t> const &position, vec3<uint16_t> const &angles);
  /// Advance unobstructed normal-player controls; collision and damaged controllers are separate
  void step(herc_controls const &controls, int16_t frame_ratio);
  locomotion_state const &movement() const;
  aiming_state const &torso() const;
  aiming_state const &elevation() const;
  model_instance const &model() const;
  weapon_assembly const &weapons() const;
  model_transform placement() const;
};

} // namespace earthsiege
