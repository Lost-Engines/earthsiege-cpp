#pragma once
#include "resources/model.h"
#include "simulation/target_selection.h"

namespace earthsiege {

enum class herc_aim_mode { centre_controls, target_centre, target_component };
struct herc_aim_selection {
  herc_aim_mode mode{herc_aim_mode::centre_controls};
  int16_t component{-1};
};
herc_aim_selection select_herc_aim_point(combat_actor_kind target_kind, vec3<int32_t> const &origin, uint16_t heading, int16_t torso_limit, vec3<int32_t> const &target_position, int16_t component);

struct herc_aim_request {
  vec3<int16_t> errors;                                                   // pitch, roll (always zero), yaw in the articulated aiming frame
  vec2<int16_t> controls;                                                 // torso, elevation; Q8 full deflection is 256
  int32_t convergence_distance{0};
};

/// Original AI steering uses two separate inversions, retaining their rounding.
herc_aim_request aim_herc_at(vec3<int32_t> const &point, model_transform const &placement, model_transform const &pivot, int16_t eye_height);
herc_aim_request centre_herc_aim(int16_t torso, int16_t elevation, int32_t convergence_distance);
/// Mount offsets converge on (0, distance, eye height) in the local aiming frame.
vec3<int16_t> herc_weapon_convergence(vec3<int16_t> const &mount_offset, int16_t eye_height, int32_t distance);

} // namespace earthsiege
