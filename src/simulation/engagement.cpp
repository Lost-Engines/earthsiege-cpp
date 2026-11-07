#include "engagement.h"
#include <algorithm>
#include <stdexcept>
#include "simulation/fixed_math.h"

namespace earthsiege {

void assign_target(std::span<target_relationship> const relationships, size_t const observer, std::optional<size_t> const selected) {
  if(observer >= relationships.size() || (selected && *selected >= relationships.size())) throw std::out_of_range{"target relationship reference"};
  auto &state{relationships[observer]};
  if(state.selected == selected) return;
  if(state.selected) {
    if(*state.selected >= relationships.size()) throw std::out_of_range{"previous target relationship reference"};
    auto &previous{relationships[*state.selected]}; previous.attackers = wrap_word(previous.attackers - 1);
  }
  state.selected = selected;
  if(selected) { auto &next{relationships[*selected]}; next.attackers = wrap_word(next.attackers + 1); }
  state.changed = true;
}

void patrol_engagement(engagement_mode const mode, engagement_state &state, size_t const actor, std::span<target_relationship> const relationships, target_objective const &objective, frame_ratio const ratio, uint16_t &world_stagger, engagement_callbacks const &callbacks, bool const weapons_free) {
  bool const flyer{mode == engagement_mode::flyer_search || mode == engagement_mode::flyer_patrol};
  bool const patrol{mode == engagement_mode::herc_patrol || mode == engagement_mode::flyer_patrol};
  auto const command{state.command};
  callbacks.navigate();
  if(!flyer) assign_target(relationships, actor, std::nullopt);
  if(actor != objective.leader && !(patrol && !flyer && command != 0) && !(weapons_free && !flyer)) return;
  auto &timer{state.controller.target_search_remaining};
  timer = std::max<int16_t>(0, wrap_word(timer - ratio));
  if(timer != 0) return;
  timer = 10000;
  bool const retreat{!flyer && (state.unavailable || state.disabled || state.dead)};
  std::optional<target_selection_flags> selection;
  if(flyer) selection = static_cast<target_selection_flags>(patrol ? 16 : 17);
  else if(retreat) selection = target_selection_flags::none;
  else if(!patrol || (command == 0 && objective.order == 8) || command == 2) selection = target_selection_flags::prefer_objective;
  if(!selection) return;
  auto const target{callbacks.select(*selection)};
  if(!patrol && !retreat && (!target || std::ranges::find(objective.actors, *target) == objective.actors.end())) return;
  assign_target(relationships, actor, target);
  if(!target) return;
  if(retreat) {
    state.aimed_component = -1;
    if(callbacks.enter) callbacks.enter(behavior::fleeing);
    else enter_behavior(state.controller, behavior::fleeing, world_stagger);
  } else {
    if(!flyer) state.attack_requested = true;
    callbacks.attack();
  }
}

void begin_flyer_attack(std::span<engagement_state> const actors, std::span<target_relationship> const relationships, size_t const actor, std::span<size_t const> const group, uint16_t &world_stagger) {
  if(actor >= actors.size() || actors.size() != relationships.size()) throw std::out_of_range{"flyer attack actor reference"};
  enter_behavior(actors[actor].controller, behavior::flyer_attacking, world_stagger);
  if(group.empty() || group.front() != actor) return;
  for(auto const member : group) {
    if(member >= actors.size()) throw std::out_of_range{"flyer attack group reference"};
    if(member == actor || actors[member].dead) continue;
    assign_target(relationships, member, relationships[actor].selected);
    enter_behavior(actors[member].controller, behavior::flyer_attacking, world_stagger);
  }
}

} // namespace earthsiege
