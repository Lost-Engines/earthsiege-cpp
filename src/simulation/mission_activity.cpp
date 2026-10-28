#include "mission_activity.h"
#include <map>
#include <stdexcept>

namespace earthsiege {
namespace {
std::optional<size_t> reference(int16_t const value) { return value < 0 ? std::nullopt : std::optional<size_t>{static_cast<size_t>(value)}; }
}
mission_activity::mission_activity(mission_script const &script, mission_placement const &placement, mission_counters initial)
  : settings{script.header}, counters{initial}, triggers{script} {
  counters[7] = 0;
  std::array<size_t,3> offsets{0,placement.actors[0].size(),placement.actors[0].size()+placement.actors[1].size()};
  auto const count{offsets[2]+placement.actors[2].size()};
  std::array<std::map<size_t,size_t>,3> sources;
  controllers.resize(count); relationships.resize(count); actors.reserve(count);
  for(size_t kind{0}; kind != 3; ++kind) for(auto const &source : placement.actors[kind]) {
    auto const id{actors.size()}; sources[kind][source.source] = id;
    mission_actor_activity state{static_cast<actor_kind>(kind),source.group,source.ordinal,source.player_controlled,false,{},{},-1,0,actor_awareness{count}};
    auto const events{[&](auto const &entry){ state.contact_trigger=reference(entry.trigger_a); state.hit_trigger=reference(entry.trigger_b); state.counter=entry.counter; state.counter_operation=entry.counter_operation; }};
    if(kind == 0) {
      if(!source.player_roster) { auto const &entry{script.hercs.at(source.source)}; events(entry); state.persistent_signature=entry.field_mask[0]!=0; }
      enter_behavior(controllers[id].controller,behavior::herc_deciding,stagger);
    } else if(kind == 1) {
      events(script.flyers.at(source.source)); enter_behavior(controllers[id].controller,behavior::flyer_deciding,stagger);
    } else events(script.bases.at(source.source));
    if(state.player) player=id;
    actors.push_back(std::move(state));
  }
  for(size_t i{0}; i != placement.groups.size(); ++i) {
    auto const &source{placement.groups[i]}; auto const &record{script.groups.at(i)};
    mission_group_activity entry{source.kind,source.argument,source.formation,record.route,0,source.mode,reference(record.trigger),{},{},{}};
    for(auto const member : source.members) entry.members.push_back(offsets.at(static_cast<size_t>(member.kind))+member.slot);
    groups.push_back(std::move(entry));
  }
  for(size_t i{0}; i != groups.size(); ++i) {
    auto const &record{script.groups[i]}; auto &entry{groups[i]};
    if(record.target_type == target_kind::none) continue;
    if(record.target_type == target_kind::group) entry.objectives=groups.at(static_cast<size_t>(record.target)).members;
    else {
      auto const kind{static_cast<size_t>(record.target_type)-1};
      auto const found{sources.at(kind).find(static_cast<size_t>(record.target))};
      // The original pool lookup returns null for an unconstructed source slot.
      if(found != sources[kind].end()) entry.objectives.push_back(found->second);
    }
    if(!entry.objectives.empty()) entry.representative=entry.objectives.front();
  }
}
mission_actor_activity const &mission_activity::actor(size_t const index) const { return actors.at(index); }
mission_actor_activity &mission_activity::actor(size_t const index) { return actors.at(index); }
mission_group_activity const &mission_activity::group(size_t const index) const { return groups.at(index); }
mission_group_activity &mission_activity::group(size_t const index) { return groups.at(index); }
std::span<mission_group_activity const> mission_activity::all_groups() const { return groups; }
std::span<engagement_state> mission_activity::decisions() { return controllers; }
std::span<target_relationship> mission_activity::targets() { return relationships; }
uint16_t &mission_activity::entry_stagger() { return stagger; }
size_t mission_activity::size() const { return actors.size(); }
bool mission_activity::waiting(size_t const index) const { return groups.at(actors.at(index).group).activation_trigger.has_value(); }
mission_counters const &mission_activity::variables() const { return counters; }
bool mission_activity::triggered(size_t const index) const { return triggers.latched(index); }
void mission_activity::activate(size_t const index) { triggers.activate(index,counters); }
target_objective mission_activity::objective(size_t const index) const {
  auto const &entry{groups.at(actors.at(index).group)};
  if(entry.members.empty()) throw std::runtime_error{"target selection requires a group leader"};
  return {entry.order,entry.objectives,entry.representative,entry.members.front()};
}
void mission_activity::dispatch_groups(std::function<void(size_t)> const &dispatch) {
  for(auto &entry : groups) {
    if(entry.activation_trigger) {
      if(triggers.latched(*entry.activation_trigger)) entry.activation_trigger.reset();
    } else for(auto const member : entry.members) dispatch(member);
  }
}
void mission_activity::scan_areas(std::span<observed_actor const> const observations) {
  if(observations.size()!=actors.size()) throw std::invalid_argument{"mission observation count"};
  std::vector<std::vector<vec3<int32_t>>> positions(groups.size());
  std::vector<trigger_group> views;
  for(size_t i{0}; i!=groups.size(); ++i) {
    auto const &entry{groups[i]};
    for(auto const member : entry.members) positions[i].push_back(vec3<int32_t>{observations[member].position});
    views.push_back({entry.kind,entry.side,entry.activation_trigger.has_value(),positions[i]});
  }
  triggers.scan(observations[player].position,actors[player].group,views,counters);
}
void mission_activity::perceive(std::span<observed_actor const> const observations, frame_ratio const ratio, random_state &random, std::function<bool(size_t,size_t)> const &line_of_sight, std::function<void(size_t,size_t)> const &noticed) {
  if(observations.size()!=actors.size()) throw std::invalid_argument{"mission observation count"};
  std::vector<perception_actor> views;
  for(size_t i{0}; i!=actors.size(); ++i) {
    auto &state{actors[i]}; auto const &input{observations[i]};
    views.push_back({vec3<int32_t>{input.position},groups[state.group].side,waiting(i),state.contact_trigger,state.awareness,input.dead,input.disabled,state.player,input.detectable});
  }
  advance_perception(views,ratio,settings.detection_range(),random,triggers,counters,{
    .line_of_sight{line_of_sight}, .orientation_offset{[&](size_t i){return observations[i].view_offset;}},
    .within_view{[&](size_t i,int16_t bearing){return actors[i].kind==actor_kind::base || within_actor_view(observations[i].heading,bearing);}}, .noticed{noticed}});
}
std::optional<size_t> mission_activity::select(std::span<observed_actor const> const observations, size_t const observer, target_selection_flags const flags, std::optional<size_t> const hud_target, random_state &random) const {
  if(observations.size()!=actors.size()) throw std::invalid_argument{"mission observation count"};
  std::vector<target_candidate> candidates;
  for(size_t i{0}; i!=actors.size(); ++i) {
    auto const &state{actors[i]}; auto const &input{observations[i]};
    candidates.push_back({.position{vec3<int32_t>{input.position}}, .heading{input.heading}, .kind{combat_kind(state.kind)},
      .side{groups[state.group].side}, .strength{input.strength}, .known{state.awareness.known}, .seen{state.awareness.seen},
      .dead{input.dead}, .removed{input.removed || waiting(i)}, .disabled{input.disabled}, .unavailable{input.unavailable}, .objective_event{input.objective_event},
      .player{state.player}, .ignore_hud_target{input.ignore_hud_target}, .target{relationships[i].selected}, .attackers{relationships[i].attackers},
      .controller_attacking{controllers[i].controller.type ? std::optional<bool>{describe(*controllers[i].controller.type).attacking()} : std::nullopt},
      .compares_combat_strength{state.kind==actor_kind::herc}});
  }
  return select_target(candidates,observer,flags,objective(observer),settings.objective_kind,hud_target,random);
}
} // namespace earthsiege
