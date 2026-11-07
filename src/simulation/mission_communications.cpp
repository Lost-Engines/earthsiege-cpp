#include "mission_session.h"
#include <utility>

namespace earthsiege {
std::optional<int16_t> mission_session::pilot_identity(size_t const actor) const { return actor<crew.size()?crew[actor].pilot_slot:std::nullopt; }
wingmate_order_state mission_session::order_state(size_t const index) const {
  wingmate_order_state s;
  s.expansion=weapon_definitions.edition==game_edition::expansion;
  auto const &decision{activity->decisions()[index]};
  s.position=combat_position(index).position; s.type=decision.controller.type.value_or(behavior::herc_deciding);
  s.dead=decision.dead; s.disabled=decision.disabled; s.unavailable=decision.unavailable;
  s.selected=activity->targets()[index].selected; s.herc=index<crew.size();
  if(s.herc) {
    auto const &member{crew[index]}; s.weapons_free=member.weapons_free;
    s.radar=member.actor->impacts().detectable; s.radar_requested=member.radar_active; s.ignore_target=member.ignore_hud_target;
    s.command=decision.command; s.designated=member.designated; s.followed=member.followed; s.stop_status=member.stop_status;
    s.command_position={member.command_position.x,member.command_position.y};
    s.incoming_damage=member.reactions.incoming_damage; s.reconsider_remaining=decision.controller.reconsider_remaining;
  }
  return s;
}
void mission_session::notice_contact(size_t observer,size_t target,frame_ratio ratio){
  // HERC override 0e760; ground/flying actors retain no-op 0e75c.
  if(observer>=crew.size())return;
  if(observer==player_index){announced_contacts.at(target)=true;return;}
  // Original 0e760 decrements this timer per notice, not once per frame.
  contact_cooldown=std::max(wrap_word(int(contact_cooldown)-ratio),int16_t{0});
  if(activity->actor(target).kind==actor_kind::base||actor_group(observer)!=actor_group(player_index))return;
  auto const &seen{activity->actor(target).awareness};
  if(announced_contacts.at(target)||seen.seen||activity->actor(observer).awareness.known.at(target)||activity->actor(player_index).awareness.known.at(target))return;
  announced_contacts.at(target)=true;
  if(!contact_cooldown){announce(observer,1);contact_cooldown=10000;}
}
void mission_session::announce(size_t actor,uint16_t message,bool allow_dead){
  if(actor>=crew.size()||(!allow_dead&&crew[actor].actor->damage().consequences.dead))return;
  radio_event event{message,actor,{},{}};
  if(radio_handler)radio_handler(event);else radio_events.push_back(event);
}
void mission_session::announce_system(uint16_t message){
  radio_event event{message,{},{},{}};
  if(radio_handler)radio_handler(event);else radio_events.push_back(event);
}
void mission_session::request_wingmate_status(size_t const actor) { announce(actor,34); }
std::vector<size_t> mission_session::wingmate_ids() const {
  std::vector<size_t> result;
  for(auto const actor:activity->group(crew[player_index].group).members)
    if(actor!=player_index && result.size()<3)result.push_back(actor);
  return result;
}
bool mission_session::transmit_order(uint16_t const command) {
  return transmit_order({command,player_index,{},{}},{});
}
bool mission_session::transmit_order(wingmate_order const &order,std::optional<size_t> const recipient) {
  auto const &group{activity->group(crew[player_index].group)};
  std::vector<wingmate_order_state> states(activity->size());
  auto const refresh{[&] { for(size_t i{};i<states.size();++i) states[i]=order_state(i); }};
  auto const apply{[&](size_t index) {
    auto const &s{states[index]}; auto &member{crew.at(index)}; auto &decision{activity->decisions()[index]};
    member.actor->impacts().detectable=s.radar; member.radar_active=s.radar_requested; member.ignore_hud_target=s.ignore_target;
    member.weapons_free=s.weapons_free;
    decision.command=s.command; member.designated=s.designated; member.followed=s.followed; member.stop_status=s.stop_status;
    member.command_position.x=s.command_position.x; member.command_position.y=s.command_position.y;
    member.reactions.incoming_damage=s.incoming_damage; decision.controller.reconsider_remaining=s.reconsider_remaining;
  }};
  refresh();
  auto const deliver{[&](size_t index,uint16_t previous) {
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
      .acknowledge{[&](uint16_t response){if(!states[index].dead) announce(index,response);}},
    },group.representative)};
    apply(index);return accepted;
  }};
  if(recipient) {
    bool const accepted{*recipient<crew.size() && *recipient!=player_index && !states[*recipient].dead && deliver(*recipient,0)};
    // 2b3e8 cancels this pilot's status request after sending an order.
    radio_event event{34,*recipient,{},{},true};
    if(radio_handler)radio_handler(event);else radio_events.push_back(event);
    return accepted;
  }
  return dispatch_wingmate_order(states,group.members,order,deliver);
}
std::vector<radio_event> mission_session::take_radio_events() {
  return std::exchange(radio_events,{});
}
} // namespace earthsiege
