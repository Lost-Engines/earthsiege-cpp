#include "mission_session.h"
#include "simulation/mission_salvage.h"
#include "simulation/fixed_math.h"
namespace earthsiege {
void mission_session::credit_defeat(size_t attacker,size_t victim){
  auto const &own{activity->actor(attacker)},&target{activity->actor(victim)};
  auto const own_side{activity->group(own.group).side},other_side{activity->group(target.group).side};
  auto const player_group{crew[player_index].group};
  if(attacker<crew.size()&&own_side!=other_side){
    if(own.group==player_group){
      if(attacker!=player_index)announce(attacker,2);
      else{
        std::optional<size_t> nearest;int32_t distance{100000};
        auto const position{combat_position(player_index).position};
        for(auto actor:activity->group(player_group).members){
          if(actor==player_index)continue;
          auto const &state{crew[actor].actor->damage().consequences};
          if(state.dead||state.disabled)continue;
          auto const d{fast_distance(combat_position(actor).position-position)};
          if(d<distance){nearest=actor;distance=d;}
        }
        if(nearest&&distance<30000)announce(*nearest,5);
      }
    }
    auto &count{crew[attacker].statistics.at(static_cast<size_t>(combat_kind(target.kind)))};count=wrap_word(count+1);
  }
  if(victim<crew.size()&&target.group==player_group&&victim!=player_index)
    announce(victim,crew[victim].actor->damage().consequences.dead?37:4,true);
}
mission_results mission_session::results()const{
  mission_results out;out.grade=result_grade();out.salvage=recovered_weapons;int32_t score{};
  auto const &player{crew.at(player_index)};
  for(auto const &member:crew){auto const &actor{*member.actor};auto const &state{actor.damage()};
    if(member.side==player.side||(!state.consequences.dead&&!state.consequences.disabled))continue;
    std::vector<salvage_weapon> weapons;
    for(auto const &w:actor.weapons().slots())if(w)weapons.push_back({w->equipment_slot,w->definition});
    auto const salvage{assess_salvage(actor.definition().damage,state,weapons,actor.profile().salvage_value,!actor.allows_salvage())};
    score=wrap_long(int64_t(score)+salvage.score);out.salvage.insert(out.salvage.end(),salvage.weapons.begin(),salvage.weapons.end());
  }
  out.recovery=recovery_value(score,mission_variables()[7]);
  for(auto index:activity->group(player.group).members){auto const &member{crew.at(index)};
    out.crew.push_back({returned_condition(member.actor->definition().damage,member.actor->damage()),member.statistics});
  }
  return out;
}
} // namespace earthsiege
