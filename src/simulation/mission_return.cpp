#include "mission_session.h"
#include "simulation/mission_salvage.h"
#include "simulation/fixed_math.h"
namespace earthsiege {
void mission_session::credit_defeat(size_t attacker,size_t victim){
  if(attacker>=crew.size())return;
  auto &member{crew[attacker]};auto const &target{activity->actor(victim)};
  if(member.side==activity->group(target.group).side)return;
  auto &count{member.statistics.at(static_cast<size_t>(combat_kind(target.kind)))};count=wrap_word(count+1);
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
