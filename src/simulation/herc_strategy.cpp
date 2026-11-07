#include "herc_strategy.h"
#include <stdexcept>

namespace earthsiege {
void reconsider_herc_attack(std::span<engagement_state> const actors, std::span<target_relationship> const targets, size_t const actor, std::span<size_t const> const group, herc_strategy_policy const &policy, bool &detectable, herc_strategy_callbacks const &callbacks) {
  auto &state{actors[actor]};
  if(!policy.leader_player || policy.radar_active) {
    if(policy.ecm_remaining == 0) detectable = true;
  } else detectable = false;
  if(state.attack_requested) state.attack_requested = false;
  else if(state.command >= 0 && state.command <= 2) {
    bool const travelling{state.command == 1 || (state.command == 0 && policy.order == 11)};
    if(travelling && state.controller.type && describe(*state.controller.type, policy.expansion).attacking()) { callbacks.resume_orders(); return; }
    assign_target(targets, actor, callbacks.select(target_selection_flags::prefer_objective));
    if(!targets[actor].selected) { callbacks.resume_orders(); return; }
  } else {
    if(!targets[actor].selected) throw std::logic_error{"HERC directed attack has no target"};
    if(actors[*targets[actor].selected].dead) assign_target(targets, actor, callbacks.select(target_selection_flags::none));
    if(!targets[actor].selected) { callbacks.resume_orders(); return; }
  }
  if(group.front() == actor) for(auto const follower : group) {
    auto const &other{actors[follower]};
    if(follower != actor && !other.disabled && !other.dead && other.controller.type && !describe(*other.controller.type, policy.expansion).attacking()) callbacks.alert_follower(follower);
  }
  state.aimed_component = -1;
  if(callbacks.retreat()) return;
  if(!targets[actor].selected) throw std::logic_error{"HERC strategy has no target"};
  auto const kind{callbacks.target_kind(*targets[actor].selected)};
  auto next{behavior::attacking_base};
  if(kind == combat_actor_kind::flyer) next = behavior::attacking_flyer;
  else if(kind == combat_actor_kind::herc) {
    // The original evaluates this deterministic retreat decision twice for HERCs.
    // A different second result would leave its behavior pointer uninitialised.
    if(callbacks.retreat()) throw std::logic_error{"HERC retreat inputs changed during strategy entry"};
    switch(callbacks.compare_strength()) {
      case combat_balance::superior: next = behavior::facing_off; break;
      case combat_balance::comparable: next = behavior::attacking; break;
      case combat_balance::inferior: next = behavior::flanking; break;
    }
    state.aimed_component = callbacks.choose_component();
  }
  callbacks.enter(next);
}
} // namespace earthsiege
