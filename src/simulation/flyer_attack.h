#pragma once
#include "simulation/flyer_motion.h"
#include "simulation/random_state.h"
#include "simulation/weapon_firing.h"

namespace earthsiege {
enum class flyer_attack_phase : int16_t { departing, approaching, circling, levelling };
struct flyer_attack_state {
  flyer_attack_phase phase{flyer_attack_phase::departing};
  int16_t requested_speed{0};
  int16_t missile_latch{0};
  int16_t refire_timer{0};
  int32_t manoeuvre_timer{0};
  bool firing_window{false};
  uint8_t contact_state{0};                                                // original actor +0x8c, cleared at the end of the circling phase
};
struct flyer_attack_target {
  vec3<int32_t> position;
  uint16_t heading{0};
  int16_t speed{0};
};
int16_t flyer_reported_speed(flyer_motion const &flight);
/// Leader attack pass; followers continue the original formation navigation.
void attack_with_flyer(flyer_motion &flight, flyer_attack_state &state, flyer_profile const &profile, flyer_attack_target const &target, weapon_catalogue const &weapons, frame_ratio ratio, random_state &random, std::function<void(weapon_shot const &)> const &fire);
} // namespace earthsiege
