#include "mission_session.h"
#include <utility>

namespace earthsiege {
wingmate_order_state mission_session::order_state(size_t const index) const {
  wingmate_order_state s;
  auto const &decision{activity->decisions()[index]};
  s.position=combat_position(index).position; s.type=decision.controller.type.value_or(behavior::herc_deciding);
  s.dead=decision.dead; s.disabled=decision.disabled; s.unavailable=decision.unavailable;
  s.selected=activity->targets()[index].selected; s.herc=index<crew.size();
  if(s.herc) {
    auto const &member{crew[index]};
    s.radar=member.actor->impacts().detectable; s.radar_requested=member.radar_active; s.ignore_target=member.ignore_hud_target;
    s.command=decision.command; s.designated=member.designated; s.stop_status=member.stop_status;
    s.command_position={member.command_position.x,member.command_position.y};
    s.incoming_damage=member.reactions.incoming_damage; s.reconsider_remaining=decision.controller.reconsider_remaining;
  }
  return s;
}
bool mission_session::transmit_order(uint16_t const command) {
  auto const &group{activity->group(crew[player_index].group)};
  std::vector<wingmate_order_state> states(activity->size());
  auto const refresh{[&] { for(size_t i{};i<states.size();++i) states[i]=order_state(i); }};
  auto const apply{[&](size_t index) {
    auto const &s{states[index]}; auto &member{crew.at(index)}; auto &decision{activity->decisions()[index]};
    member.actor->impacts().detectable=s.radar; member.radar_active=s.radar_requested; member.ignore_hud_target=s.ignore_target;
    decision.command=s.command; member.designated=s.designated; member.stop_status=s.stop_status;
    member.command_position.x=s.command_position.x; member.command_position.y=s.command_position.y;
    member.reactions.incoming_damage=s.incoming_damage; decision.controller.reconsider_remaining=s.reconsider_remaining;
  }};
  refresh(); wingmate_order const order{command,player_index,{},{}};
  return dispatch_wingmate_order(states,group.members,order,[&](size_t index,uint16_t previous) {
    bool const accepted{receive_wingmate_order(states,index,order,previous,player_index,group.members.front(),group.order,displayed_target,{
      .select{[&](bool prefer) {
        apply(index);
        return activity->select(observe(),index,prefer?target_selection_flags::prefer_objective:target_selection_flags::none,displayed_target,random);
      }},
      .attack{[&](size_t target) {
        apply(index); assign_target(activity->targets(),index,target);
        switch(activity->actor(target).kind) {
          case actor_kind::herc: activity->decisions()[index].attack_requested=true; reconsider_herc(index); break;
          case actor_kind::base: enter_herc(index,behavior::attacking_base); break;
          case actor_kind::flyer: enter_herc(index,behavior::attacking_flyer); break;
        }
        refresh();
      }},
      .resume{[&]{apply(index);resume_herc_orders(index);refresh();}},
      .enter{[&](behavior type){apply(index);enter_herc(index,type);refresh();}},
      .acknowledge{[&](uint16_t response){if(!states[index].dead) radio_responses.push_back({index,response});}},
    })};
    apply(index);return accepted;
  });
}
std::vector<wingmate_radio_response> mission_session::take_radio_responses() {
  return std::exchange(radio_responses,{});
}
} // namespace earthsiege
