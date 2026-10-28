#include "mission_objectives.h"
#include <algorithm>
#include <array>
#include <stdexcept>
#include "simulation/fixed_math.h"
#include "simulation/model_math.h"

namespace earthsiege {
namespace {
vec3<int32_t> delta(vec3<int32_t> const &a, vec3<int32_t> const &b) {
  return {wrap_long(static_cast<int64_t>(a.x)-b.x),wrap_long(static_cast<int64_t>(a.y)-b.y),wrap_long(static_cast<int64_t>(a.z)-b.z)};
}
int32_t horizontal_distance(vec3<int32_t> const &a, vec3<int32_t> const &b) {
  auto const d{delta(a,b)}; return fast_distance_2d({d.x,d.y});
}
bool terminal(mission_status const status) {
  switch(status) {
    case mission_status::dead: case mission_status::disabled: case mission_status::objective_critical:
    case mission_status::boundary_warning: case mission_status::outside_area: case mission_status::complete:
    case mission_status::abandoned_disabled: return true;
    default: return false;
  }
}
}
mission_bounds mission_bounds::from_points(std::span<vec3<int32_t> const> const points) {
  // Original bounds start at the BSS zero values, and include every SCRIPT point.
  mission_bounds result;
  for(auto const &p:points) for(unsigned axis{0};axis!=2;++axis) {
    result.minimum[axis]=std::min(result.minimum[axis],p[axis]);
    result.maximum[axis]=std::max(result.maximum[axis],p[axis]);
  }
  return result;
}
bool mission_bounds::outside(vec3<int32_t> const &p,int32_t const margin) const {
  for(unsigned axis{0};axis!=2;++axis)
    if(p[axis]<=wrap_long(static_cast<int64_t>(minimum[axis])-margin) || p[axis]>=wrap_long(static_cast<int64_t>(maximum[axis])+margin)) return true;
  return false;
}
int16_t objective_world::actor_grade(size_t const index) const {
  auto const &actor{actors[index]}; if(actor.dead || actor.removed) return 4;
  return actor.damage_fraction<50 ? 0 : actor.damage_fraction<128 ? 1 : actor.damage_fraction<192 ? 2 : 3;
}
int16_t objective_world::group_grade(size_t const index) const {
  auto const &members{groups[index].members};
  if(members.empty()) throw std::invalid_argument{"mission grade requires a nonempty group"};
  int16_t total{0};int disabled{0};
  for(auto const member:members) {total=wrap_word(total+actors[member].damage_fraction);disabled+=actors[member].dead || actors[member].disabled;}
  int const count{static_cast<int>(members.size())},average{total/count};
  if(disabled==count) return 4;
  int const fraction{disabled*1024/count};
  return fraction>=500 || average>=193 ? 3 : fraction>=250 || average>=129 ? 2 : average>=51 ? 1 : 0;
}
int16_t objective_world::target_grade(size_t const group) const {
  auto const &target{groups[group]};
  if(target.target_group) return group_grade(*target.target_group);
  if(target.target_actor) return actor_grade(*target.target_actor);
  throw std::invalid_argument{"mission objective has no target"};
}
bool objective_world::safe(size_t const index) const {
  auto const &actor{actors[index]};auto const &own_group{groups[actor.group]};
  for(size_t other{0};other!=actors.size();++other) {
    auto const &enemy{actors[other]};auto const &enemy_group{groups[enemy.group]};
    if(enemy_group.waiting || enemy_group.side==own_group.side || enemy.dead || enemy.disabled || enemy.unavailable) continue;
    auto const distance{fast_distance(delta(enemy.position,actor.position))};
    bool const recognized{(actor.seen && distance<1000000) || (index<enemy.known.size() && enemy.known[index]!=0)};
    bool const objective{enemy_group.target_actor==index || enemy_group.target_group==actor.group};
    if(actor.group!=actors[player].group && objective && (!enemy_group.route_complete || recognized)) return false;
    if(recognized && distance<(enemy.selected==index ? 100000 : 80000)) return false;
  }
  return true;
}
bool objective_world::target_safe(size_t const group) const {
  auto const &target{groups[group]};
  if(target.target_group) {
    auto const &g{groups[*target.target_group]}; if(g.waiting) return false;
    return std::ranges::all_of(g.members,[&](size_t member){return actors[member].dead || safe(member);});
  }
  if(target.target_actor) return !groups[actors[*target.target_actor].group].waiting && safe(*target.target_actor);
  throw std::invalid_argument{"mission objective has no target"};
}
bool objective_world::target_route_complete(size_t const group) const {
  auto const &target{groups[group]};
  if(target.target_group) return groups[*target.target_group].route_complete;
  if(target.target_actor) return groups[actors[*target.target_actor].group].route_complete;
  throw std::invalid_argument{"mission objective has no target"};
}
mission_status objective_world::status(int16_t const kind,bool const requested) const {
  auto const &actor{actors[player]};auto const &group{groups[actor.group]};
  if(actor.dead) return mission_status::dead;
  if(actor.disabled) return mission_status::disabled;
  if(group.order==6 && target_grade(actor.group)>2) return safe(player) ? mission_status::objective_critical : mission_status::threatened;
  if(!requested && bounds.outside(actor.position,170000)) return mission_status::outside_area;
  if(!requested && bounds.outside(actor.position,60000)) return mission_status::boundary_warning;
  bool complete{false};
  switch(kind) {
    case 0: complete=actor.arrived;break;
    case 1: case 2: complete=group.route_complete;break;
    case 3: complete=actor.rescued;break;
    case 4: case 5: complete=target_safe(actor.group);break;
    case 6: case 8: case 9: complete=target_grade(actor.group)==4;break;
    case 7: complete=target_route_complete(actor.group) && target_safe(actor.group);break;
    default: throw std::invalid_argument{"unknown mission objective kind"};
  }
  if(complete) return safe(player) ? mission_status::complete : mission_status::complete_under_threat;
  if(!safe(player)) return mission_status::threatened;
  static constexpr std::array<mission_status,10> incomplete{
    mission_status::reconnaissance_incomplete,mission_status::route_incomplete,mission_status::route_incomplete,
    mission_status::rescue_incomplete,mission_status::protection_incomplete,mission_status::escort_incomplete,
    mission_status::attack_incomplete,mission_status::escort_incomplete,mission_status::attack_incomplete,mission_status::attack_incomplete};
  return incomplete[static_cast<size_t>(kind)];
}
int16_t objective_world::result_grade(size_t const index,int16_t const kind) const {
  auto const &group{groups[index]};int const own{group_grade(index)},g{std::min(own,3)};
  auto const any{[&](auto field){return std::ranges::any_of(group.members,[&](size_t i){return actors[i].*field;});}};
  int result{0};
  switch(kind) {
    case 0: result=own==4 || !any(&objective_actor::arrived) ? g+6 : !any(&objective_actor::alerted) ? 1 : g+2;break;
    case 1: result=!group.route_complete ? g+6 : g==0 && any(&objective_actor::route_checked) ? 1 : g+2;break;
    case 2: result=!group.route_complete ? g+6 : g==0 && target_grade(index)==4 ? 1 : g+2;break;
    case 3: result=!actors[group.members.front()].dead && any(&objective_actor::rescued) ? g+1 : g+5;break;
    case 4: {auto const target{target_grade(index)};result=!target_safe(index) || target>2 ? g+5 : target==0 && g<2 ? 1 : std::max(g,1)+1;break;}
    case 5: result=!target_safe(index) || !any(&objective_actor::arrived) || target_grade(index)==4 ? g+5 : g+1;break;
    case 6: result=target_grade(index)!=4 ? g+6 : !any(&objective_actor::alerted) && g==0 ? 1 : g+2;break;
    case 7: {auto const target{target_grade(index)};result=!target_safe(index) || target==4 ? g+5 : target==0 && g<2 ? 1 : target<2 && g<3 ? 2 : target<3 && g<3 ? 3 : 4;break;}
    case 8: {auto const target{target_grade(index)};result=target==4 ? g+1 : target>=1 && g==0 ? 5 : g+5;break;}
    case 9: {auto const target{target_grade(index)};result=target==4 && g==0 ? 1 : target<2 ? (target==1 && g<2 ? g+5 : std::max(g,2)+5) : std::max(g,1)+1;break;}
    default: throw std::invalid_argument{"unknown mission objective kind"};
  }
  return static_cast<int16_t>(result-1);
}
std::optional<mission_status> mission_outcome_clock::advance(frame_ratio const ratio,std::function<mission_status()> const &evaluate) {
  confirm_remaining=std::max(int16_t{0},wrap_word(confirm_remaining-ratio));
  poll_remaining=std::max(int16_t{0},wrap_word(poll_remaining-ratio));
  if(poll_remaining!=0) return std::nullopt;
  auto const current{evaluate()};
  if(terminal(current) && current!=reported) {
    if(!pending) {pending=true;confirm_remaining=10000;}
    if(confirm_remaining==0 || current==mission_status::dead) {reported=current;return current;}
  }
  poll_remaining=10000;return std::nullopt;
}
void advance_player_objective(player_objective_progress &progress,player_objective_input const &input,mission_route const *route,std::span<vec3<int32_t> const> const points,int16_t &cursor,frame_ratio const ratio,std::function<void(int16_t)> const &announce) {
  if(!input.leader) return;
  auto const next{static_cast<size_t>(cursor+1)};
  if(route && next<route->points.size() && horizontal_distance(input.position,points[static_cast<size_t>(route->points[next])])<10000) {
    // The player does not advance onto the repeated start point of a closed route.
    bool const closing{next==route->points.size()-1 && route->points[next]==route->points.front()};
    if(!closing) {announce(0);++cursor;}
  }
  if(input.kind==0) {
    if(!progress.target_announced && input.selected_objective) {progress.target_announced=true;announce(5);progress.arrived=true;}
  } else if(input.kind==5) {
    if(horizontal_distance(input.position,input.anchor)<40000) progress.arrived=true;
  } else if(input.kind==3 && progress.rescue_stage!=4) {
    bool in_range{false};
    if(input.rescue_position && !input.target_dead && !input.target_waiting && fast_distance(delta(*input.rescue_position,input.position))<10000) {
      // Heading subtraction wraps before the wider torso/cone addition.
      auto const error{wrap_word(bearing_angle(input.position,*input.rescue_position)-input.heading)};
      in_range=static_cast<uint32_t>(static_cast<int32_t>(error)+input.torso+8192)<16384;
    }
    if(in_range) {
      progress.rescue_remaining=std::max(0,wrap_long(static_cast<int64_t>(progress.rescue_remaining)-ratio));
      if(progress.rescue_remaining!=0) return;
      // Original signed longs at 0x91ce8: the third interval is negative, so the
      // final acknowledgement becomes eligible on the following update.
      static constexpr std::array<int32_t,4> intervals{5000,5000,-25536,0};
      progress.rescue_remaining=intervals.at(static_cast<size_t>(progress.rescue_stage));
      announce(static_cast<int16_t>(29+progress.rescue_stage));
      if(++progress.rescue_stage==4) progress.rescued=true;
    } else {
      if(progress.rescue_stage!=0) {announce(33);progress.rescue_stage=0;}
      progress.rescue_remaining=0;
    }
  }
}
} // namespace earthsiege
