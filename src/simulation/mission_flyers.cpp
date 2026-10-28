#include "simulation/mission_session.h"
#include "simulation/fixed_math.h"
#include "simulation/flyer_navigation.h"
#include "simulation/model_math.h"

namespace earthsiege {
std::vector<observed_actor> mission_session::observe() const {
  std::vector<observed_actor> result;
  for(size_t index{0}; index != crew.size(); ++index){
    auto const &member{crew[index]};
    auto const &body{member.actor->body()};auto const &damage{member.actor->damage().consequences};
    result.push_back({.position{vec3<int32_t>{body.movement().position}},.heading{body.movement().angles.z},.view_offset{body.torso().angle}, .strength{member.actor->combat_rating()},
      .dead{damage.dead},.disabled{damage.disabled},.detectable{member.actor->impacts().detectable},
      .removed{body.movement().collapsed},.unavailable{activity->decisions()[index].unavailable},.objective_event{member.objective_event}});
  }
  for(auto const &member:flyers)result.push_back({.position{vec3<int32_t>{member.actor->motion().position}},.heading{member.actor->angles().z},.dead{member.actor->dead()},.detectable{member.actor->is_detectable()}});
  for(auto const &member:bases)result.push_back({.position{vec3<int32_t>{member.actor->placement().translation}},.heading{transform_angles(member.actor->placement()).z},.dead{member.actor->dead()},.detectable{member.actor->is_detectable()}});
  return result;
}
int16_t mission_session::actor_speed(size_t const index) const {
  if(index<crew.size())return wrap_word((2000*crew[index].actor->body().movement().gait_rate+512)>>10);
  if(index<base_id(0))return flyer_reported_speed(flyers[index-flyer_id(0)].actor->motion());
  return 0;
}
std::optional<int32_t> mission_session::obstacle_distance(avoidance_probe const kind,vec3<int32_t> const &start,vec3<int32_t> const &end) const {
  if(kind==avoidance_probe::scenery){auto const hit{intersect_scenery_segment(terrain.colliders(),start,end)};return hit?std::optional<int32_t>{hit->distance}:std::nullopt;}
  std::vector<scenery_collider> buildings;
  for(auto const &member:bases)if(auto const shape{member.actor->obstruction()})buildings.push_back(*shape);
  auto const hit{intersect_scenery_segment(buildings,start,end)};return hit?std::optional<int32_t>{hit->distance}:std::nullopt;
}
void mission_session::navigate_flyer(size_t const index){
  auto &member{flyers.at(index)};auto &actor{*member.actor};auto const id{flyer_id(index)};
  auto &group{activity->group(member.group)};
  if(group.members.front()==id){
    if(group.route<0)throw std::runtime_error{"active flyer route is absent"};
    follow_flyer_route(actor.motion(),actor.profile(),routes.at(static_cast<size_t>(group.route)),points,group.route_cursor,member.attack.requested_speed);
  }else{
    auto const &leader{*flyers.at(group.members.front()-flyer_id(0)).actor};
    auto const ordinal{activity->actor(id).member};
    auto const &offset{layout.formation_offset(actor_kind::flyer,static_cast<size_t>(group.formation),ordinal-1)};
    follow_flyer_formation(actor.motion(),actor.profile(),leader.motion(),offset,member.attack.requested_speed);
  }
  actor.set_detectable(false);
}
void mission_session::dispatch_flyer(size_t const index,frame_ratio const ratio){
  auto &member{flyers.at(index)};auto &actor{*member.actor};auto const id{flyer_id(index)};
  auto &state{activity->decisions()[id]};auto &group{activity->group(member.group)};
  auto const formation{[&]{
    auto const &leader{*flyers.at(group.members.front()-flyer_id(0)).actor};
    auto const &offset{layout.formation_offset(actor_kind::flyer,static_cast<size_t>(group.formation),activity->actor(id).member-1)};
    follow_flyer_formation(actor.motion(),actor.profile(),leader.motion(),offset,member.attack.requested_speed);
  }};
  dispatch_controller(state.controller,member.skip_decision,ratio,{
    .reconsider{[&](behavior type){if(type!=behavior::flyer_dead && type!=behavior::flyer_sleeping){ enter_behavior(state.controller,initial_flyer_behavior(group.order),activity->entry_stagger()); member.attack.missile_latch=0; }}},
    .advance{[&](behavior type){
      if(type==behavior::flyer_attacking || type==behavior::flyer_patrolling || type==behavior::flyer_search_destroy || type==behavior::flyer_scouting)
        advance_flyer(actor.motion(),actor.profile(),ratio,[&](avoidance_probe kind,auto const &start,auto const &end){return obstacle_distance(kind,start,end);});
    }},
    .decide{[&](behavior type){
      if(type==behavior::flyer_scouting){navigate_flyer(index);return false;}
      if(type==behavior::flyer_patrolling || type==behavior::flyer_search_destroy){
        patrol_engagement(type==behavior::flyer_patrolling?engagement_mode::flyer_patrol:engagement_mode::flyer_search,state,id,activity->targets(),activity->objective(id),ratio,activity->entry_stagger(),{
          .navigate{[&]{navigate_flyer(index);}},
          .select{[&](target_selection_flags flags){return activity->select(observe(),id,flags,std::nullopt,random);}},
          .attack{[&]{
            auto const inputs{observe()};for(size_t i{0};i!=inputs.size();++i)activity->decisions()[i].dead=inputs[i].dead;
            begin_flyer_attack(activity->decisions(),activity->targets(),id,group.members,activity->entry_stagger());
            member.attack.missile_latch=0;
            if(group.members.front()==id) for(auto const follower:group.members)
              if(follower!=id && !inputs[follower].dead) flyers.at(follower-flyer_id(0)).attack.missile_latch=0;
          }}});
      }else if(type==behavior::flyer_attacking){
        auto const selected{activity->targets()[id].selected};
        if(!selected)throw std::runtime_error{"attacking flyer has no target"};
        auto const inputs{observe()};auto const &target{inputs.at(*selected)};
        if(target.dead || target.disabled)return true;
        if(group.members.front()!=id)formation();
        else attack_with_flyer(actor.motion(),member.attack,actor.profile(),{vec3<int32_t>{target.position},target.heading,actor_speed(*selected)},weapon_definitions,ratio,random,[&](weapon_shot const &shot){
          launch(id, shot);
        });
      }
      return false;
    }}});
}

void mission_session::advance_base(size_t const index, frame_ratio const ratio) {
  auto &actor{*bases.at(index).actor};
  if(!actor.is_turret() || actor.dead()) { actor.advance(ratio, random, base_damage_events(index)); return; }
  auto const id{base_id(index)};
  actor.advance_turret(ratio, weapon_definitions, {
    .select{[&]{return activity->select(observe(), id, static_cast<target_selection_flags>(0x30), std::nullopt, random);}},
    .target{[&](size_t target){
      auto const observations{observe()};
      vec3<int16_t> offset;
      if(target < crew.size()) offset = crew[target].definition->profile.camera_focus;
      else if(target >= base_id(0)) offset.z = bases.at(target-base_id(0)).actor->profile().target_height;
      // The original adds this authored offset in world axes, before lead.
      return turret_target{vec3<int32_t>{observations.at(target).position}, vec3<int16_t>{offset}, {0, 0, observations.at(target).heading}, actor_speed(target)};
    }},
    .fire{[&](weapon_shot const &shot){launch(id, shot);}},
  });
}

vec3<int32_t> mission_session::target_position(size_t const index, int16_t const component) const {
  if(index < crew.size()) return crew[index].actor->target_position(component);
  if(index < base_id(0)) return vec3<int32_t>{flyers[index-flyer_id(0)].actor->motion().position};
  return bases.at(index-base_id(0)).actor->target_position();
}
void mission_session::launch(size_t const owner, weapon_shot const &shot) {
  projectile_owner source{owner, activity->targets()[owner].selected};
  if(shot.kind == shot_kind::rocket && source.target && *source.target < crew.size())
    source.target_component = crew[*source.target].actor->target_component(shot.muzzle.position, shot.muzzle.angles);
  projectiles.launch(source, shot, random, [&](projectile_owner const &owner_info, impact_request const &request){ return impact(owner_info, request); });
}
} // namespace earthsiege
