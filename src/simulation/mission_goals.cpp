#include "mission_session.h"
#include <algorithm>
#include <utility>
#include "simulation/fixed_math.h"

namespace earthsiege {
std::pair<std::vector<objective_actor>,std::vector<objective_group>> mission_session::objective_facts() const {
  std::vector<objective_actor> actors;std::vector<objective_group> groups;
  auto const observations{observe()};
  for(size_t i{0};i!=observations.size();++i) {
    auto const &input{observations[i]};auto const &identity{activity->actor(i)};
    objective_actor actor{.position{vec3<int32_t>{input.position}},.group{identity.group},
      .dead{input.dead},.disabled{input.disabled},.removed{input.removed},.unavailable{input.unavailable},.seen{identity.awareness.seen},
      .alerted{identity.awareness.alerted},.selected{activity->targets()[i].selected},.known{identity.awareness.known}};
    if(i<crew.size()) {
      actor.damage_fraction=crew[i].actor->total_damage_fraction();
      actor.arrived=crew[i].objectives.arrived;actor.rescued=crew[i].objectives.rescued;actor.route_checked=crew[i].route_checked;
    } else if(i<base_id(0)) actor.damage_fraction=flyers[i-flyer_id(0)].actor->total_damage_fraction();
    else {
      auto const &base{*bases[i-base_id(0)].actor};
      actor.damage_fraction=wrap_word(static_cast<int32_t>(base.damage())*256/base.profile().damage_capacity);
    }
    actors.push_back(actor);
  }
  for(auto const &source:activity->all_groups()) {
    bool const complete{source.route<0 || static_cast<size_t>(source.route_cursor+1)>=routes.at(static_cast<size_t>(source.route)).points.size()};
    groups.push_back({.order{source.order},.side{source.side},.waiting{source.activation_trigger.has_value()},.route_complete{complete},.members{source.members},
      .target_actor{source.objective_group ? std::nullopt : source.representative},.target_group{source.objective_group}});
  }
  return {std::move(actors),std::move(groups)};
}
mission_status mission_session::mission_state(bool const requested) const {
  auto const [actors,groups]{objective_facts()};
  return objective_world{actors,groups,player_index,mission_bounds::from_points(points)}.status(settings.objective_kind,requested);
}
int16_t mission_session::result_grade() const {
  auto const [actors,groups]{objective_facts()};
  return objective_world{actors,groups,player_index,mission_bounds::from_points(points)}.result_grade(crew[player_index].group,settings.objective_kind);
}
mission_outcome_clock const &mission_session::mission_clock() const {return outcome_clock;}
player_objective_progress const &mission_session::objective_progress() const {return crew[player_index].objectives;}
std::optional<mission_status> mission_session::take_mission_prompt() {return std::exchange(pending_mission_prompt,std::nullopt);}
void mission_session::advance_player_objectives(size_t const index,frame_ratio const ratio) {
  auto &member{crew[index]};auto &group{activity->group(member.group)};auto const &body{member.actor->body()};
  auto const selected{activity->targets()[index].selected};
  player_objective_input input{.leader{group.members.front()==index},.kind{settings.objective_kind},.position{vec3<int32_t>{body.movement().position}},
    .heading{body.movement().angles.z},.torso{body.torso().angle},.selected_objective{selected && std::ranges::find(group.objectives,*selected)!=group.objectives.end()},
    .anchor{}};
  if(input.leader && input.kind==5) input.anchor=guard_anchor(index);
  if(group.representative) {
    auto const target{*group.representative};auto const observation{observe().at(target)};
    input.rescue_position=vec3<int32_t>{observation.position};input.target_dead=observation.dead;input.target_waiting=activity->waiting(target);
  }
  advance_player_objective(member.objectives,input,group.route<0 ? nullptr : &routes.at(static_cast<size_t>(group.route)),points,group.route_cursor,ratio,[&](int16_t event){
    frame_notices.push_back({combat_notice_kind::mission_event,index,0,event,{}});
  });
}
} // namespace earthsiege
