#pragma once
#include <functional>
#include "simulation/controller.h"
#include "simulation/navigation.h"

namespace earthsiege {

struct herc_target_geometry {
  int16_t bearing{0};
  int16_t bearing_error{0};
  int16_t bearing_magnitude{0};
  int16_t facing_error{0};
  int16_t facing_magnitude{0};
  int32_t distance{0};
  int16_t direction{0};
  int32_t near_distance{15000};
  int32_t far_distance{30000};
};

struct herc_combat_position {
  vec3<int32_t> position;
  uint16_t heading{0};
  int16_t view_offset{0};
};

/// Scratch state shared by the attack/facing/flanking controllers; reset on entry.
struct herc_attack_state {
  int16_t last_direction{0};
  int16_t steering_memory{0};                                               // original +0x57, cleared when the torso centres behind a target
  int16_t reverse_remaining{0};
  int16_t damage_check_remaining{0};
  bool behind_target{false};
};

herc_target_geometry relative_herc_target(herc_combat_position const &actor, herc_combat_position const &target);
movement_request herc_range_movement(herc_target_geometry &geometry, int16_t &last_direction);

struct herc_attack_inputs {
  herc_combat_position actor;
  herc_combat_position target;
  int32_t received_damage{0};
  bool actor_incapacitated{false};
  bool target_incapacitated{false};
  bool target_completed{false};                                            // separate objective/command completion policy
  bool base_can_retaliate{false};
};
struct herc_attack_callbacks {
  std::function<void(movement_request const &)> move;
  std::function<void(int16_t)> aim;                                         // signed target-relative facing, used by weapon selection
};

/// Obstruction/skirting entry precedes this pass. Return whether the controller
/// should reconsider, preserving the original aim/fire and movement call order.
bool attack_with_herc(behavior type, herc_attack_state &state, int16_t &command, herc_attack_inputs const &input, frame_ratio ratio, herc_attack_callbacks const &callbacks);

} // namespace earthsiege
