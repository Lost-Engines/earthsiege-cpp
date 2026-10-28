#pragma once
#include "simulation/engagement.h"

namespace earthsiege {
struct herc_strategy_policy {
  int16_t order{0};
  bool leader_player{false};
  bool radar_active{false};
  int16_t ecm_remaining{0};
};
struct herc_strategy_callbacks {
  std::function<std::optional<size_t>(target_selection_flags)> select;
  std::function<combat_actor_kind(size_t)> target_kind;
  std::function<void(size_t)> alert_follower;
  std::function<void()> resume_orders;
  std::function<bool()> retreat;
  std::function<combat_balance()> compare_strength;
  std::function<int16_t()> choose_component;
  std::function<void(behavior)> enter;
};

/// Reconsider the attack without copying the leader's selected target to followers.
void reconsider_herc_attack(std::span<engagement_state> actors, std::span<target_relationship> targets, size_t actor, std::span<size_t const> group, herc_strategy_policy const &policy, bool &detectable, herc_strategy_callbacks const &callbacks);

} // namespace earthsiege
