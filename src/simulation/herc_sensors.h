#pragma once
#include <array>
#include <optional>
#include "simulation/herc_impact.h"
#include "simulation/herc_weapon_selection.h"

namespace earthsiege {
struct herc_sensor_state {
  std::array<int16_t, missile_lock_channels> acquisition_remaining{};
  std::array<bool, missile_lock_channels> locks{};
  int16_t interference_remaining{0};
  int16_t damage_window_remaining{0};
  bool interfered{false};
  bool ecm_active{false};
  bool hud_locked{false};
};
struct herc_sensor_target {
  vec3<int32_t> position;
  bool herc{false};
  bool detectable{false};
  bool ecm_active{false};
};
struct herc_sensor_inputs {
  vec3<int32_t> position;
  uint16_t heading{0};
  int16_t torso{0};
  bool player{false};
  bool ecm_active{false};
  bool ecm_pulse{false};
  std::array<int16_t, missile_lock_channels> missile_counts{};
  std::optional<size_t> selected_channel;
  std::optional<herc_sensor_target> target;
};
/// Post-perception lock acquisition, ECM interference and damage-window clocks.
/// The target-change latch clears only while there is a target outside the lock
/// gate; the original fourth/fifth channel asymmetries are preserved.
void advance_herc_sensors(herc_sensor_state &state, herc_impact_state &impact, bool &target_changed, herc_sensor_inputs const &input, frame_ratio ratio, random_state &random, std::function<void()> const &notify_target = {}, std::function<void()> const &update_hud = {});
} // namespace earthsiege
