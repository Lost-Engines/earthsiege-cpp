#pragma once
#include "simulation/herc_tactics.h"
#include "simulation/engagement.h"

namespace earthsiege {
struct herc_guard_policy {
  vec3<int32_t> anchor;
  bool leader_player{false};
  bool radar_active{false};
  bool persistent_signature{false};
  bool group_has_objective{false};
  std::optional<size_t> designated;
};
struct herc_guard_callbacks {
  std::function<herc_combat_position(size_t)> position;
  std::function<bool(size_t)> eligible;
  std::function<void()> navigate;
  std::function<void()> centre_aim;
  std::function<void(vec3<int32_t> const &)> move_to;
  std::function<void(movement_request const &)> move;
  std::function<int16_t()> choose_component;
  std::function<void(behavior)> enter;
};
void guard_with_herc(std::span<engagement_state> actors, std::span<target_relationship> targets, size_t actor, std::span<size_t const> candidates, herc_guard_policy const &policy, bool &detectable, frame_ratio ratio, herc_guard_callbacks const &callbacks);

struct herc_drive_callbacks {
  std::function<bool()> obstruction_entry;
  std::function<bool()> face_off;
  std::function<void(movement_request const &)> move;
  std::function<void(int16_t)> aim;
  std::function<bool()> target_completed;
  std::function<bool()> incapacitated;
};
bool drive_off_enemy(herc_combat_position const &actor, herc_combat_position const &target, vec3<int32_t> const &anchor, std::optional<size_t> &designated, int16_t &aim_tolerance, herc_drive_callbacks const &callbacks);

} // namespace earthsiege
