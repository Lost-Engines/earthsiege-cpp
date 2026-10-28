#pragma once
#include "simulation/engagement.h"

namespace earthsiege {
struct herc_reaction_state {
  int32_t retaliate_remaining{0};
  int32_t warning_remaining{0};
  int32_t damage_remaining{0};
  int32_t incoming_damage{0};
};
struct herc_reaction_policy {
  bool player{false};
  bool player_group{false};
  bool attacker_player{false};
  bool friendly{false};
  bool leader_player{false};
  bool leader_targets_attacker{false};
  combat_actor_kind attacker_kind{combat_actor_kind::herc};
};
struct herc_reaction_callbacks {
  std::function<void()> share_contact;
  std::function<void()> friendly_warning;
  std::function<void()> under_attack;
  std::function<std::optional<size_t>()> select;
  std::function<void()> resume_orders;
  std::function<void()> strategy;
};

void warn_friendly_fire(herc_reaction_state &state, std::function<void()> const &notify);
void advance_herc_reactions(herc_reaction_state &state, frame_ratio ratio);
/// Retaliation preserves the distinct player-wingmate damage budget and cooldown.
void react_to_herc_hit(herc_reaction_state &state, engagement_state &decision, std::span<target_relationship> targets, size_t observer, size_t attacker, int16_t damage, herc_reaction_policy const &policy, bool &ignore_hud_target, std::optional<size_t> &designated, herc_reaction_callbacks const &callbacks);
} // namespace earthsiege
