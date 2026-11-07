#include "herc_reactions.h"
#include <algorithm>
#include "simulation/fixed_math.h"

namespace earthsiege {
void warn_friendly_fire(herc_reaction_state &state, std::function<void()> const &notify) {
  if(state.warning_remaining != 0) return;
  state.warning_remaining = 40000;
  if(notify) notify();
}
void advance_herc_reactions(herc_reaction_state &state, frame_ratio const ratio) {
  auto const tick{[&](int32_t &remaining){remaining = std::max(0, wrap_long(static_cast<int64_t>(remaining) - ratio));}};
  tick(state.retaliate_remaining); tick(state.warning_remaining); tick(state.damage_remaining);
  if(state.damage_remaining == 0) state.incoming_damage = 0;
}
void react_to_herc_hit(herc_reaction_state &state, engagement_state &decision, std::span<target_relationship> const targets, size_t const observer, size_t const attacker, int16_t const damage, herc_reaction_policy const &policy, bool &ignore_hud_target, std::optional<size_t> &designated, herc_reaction_callbacks const &callbacks) {
  if(policy.attacker_player && policy.player_group) warn_friendly_fire(state, callbacks.friendly_warning);
  if(policy.friendly || (policy.expansion && policy.attacker_incapacitated)) return;
  callbacks.share_contact();
  state.damage_remaining = 30000;
  if(policy.player_group) {
    state.incoming_damage = wrap_long(static_cast<int64_t>(state.incoming_damage) + damage);
    if(state.incoming_damage <= 7999) return;
  }
  if(policy.player || state.retaliate_remaining != 0) return;
  auto const &behavior{describe(decision.controller.type.value(), policy.expansion)};
  if(targets[observer].selected == attacker && behavior.attacking()) return;
  state.retaliate_remaining = 10000;
  if(!behavior.attacking() && policy.leader_player) {
    if(policy.attacker_kind == combat_actor_kind::herc) callbacks.under_attack();
    if(policy.leader_targets_attacker) ignore_hud_target = false;
  }
  if(behavior.properties & 4) {
    if(policy.leader_player && (!policy.expansion || decision.command == 0)) {decision.command = 4; designated.reset();}
    decision.controller.reconsider_remaining = 0;
  } else if(!(behavior.properties & 8)) {
    auto const old{targets[observer].selected};
    assign_target(targets, observer, callbacks.select());
    if(targets[observer].selected) {decision.attack_requested = true; callbacks.strategy();}
    else if(old) callbacks.resume_orders();
  }
}
} // namespace earthsiege
