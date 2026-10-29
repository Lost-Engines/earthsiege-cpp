#include "campaign_return.h"
#include "simulation/fixed_math.h"
#include <algorithm>
#include <stdexcept>
namespace earthsiege {
int16_t pilot_rank(int16_t total) {
  constexpr std::array<int16_t,5> thresholds{0,1,5,15,30};
  for(int16_t i{};i<5;++i)if(total<=thresholds[i])return i;
  return 4;
}
void advance_technology(technology_state &technology,int16_t missions,int16_t kind,int16_t grade) {
  // GO's result classification tables: only classes 0/1 enable early awards.
  bool const route{kind==0||kind==1||kind==2||kind==6};
  bool const favorable{missions==0||(grade>=0&&grade<(route?5:4))};
  for(size_t i{};i<technology.entries.size();++i){auto &t{technology.entries[i]};
    bool const early{i<8?t.early<=missions:t.early==missions},late{i<8?t.late<=missions:t.late==missions};
    if(!t.available&&t.early!=-1&&t.late!=-1&&((early&&favorable)||late)){t.available=1;technology.last_type=i>=8;technology.last_index=static_cast<int16_t>(i>=8?i-8:i);}
  }
}
namespace {
void push(armory &inventory,int16_t weapon,int16_t condition){auto &stack{inventory.at(static_cast<size_t>(weapon))};stack.insert(stack.begin(),condition);}
crew_member replacement(crew_member const &old,size_t member,std::array<int16_t,12> const &names,random_state &random) {
  crew_member c;c.skill=random.bounded(2);c.experience=static_cast<int16_t>(random.bounded(5)+c.skill*5);
  c.total[0]=static_cast<int16_t>((random.bounded(3)+1)*c.experience);
  c.total[2]=static_cast<int16_t>((random.bounded(2)+1)*c.experience);
  c.total[1]=static_cast<int16_t>(random.bounded(2)*c.experience);
  c.name=names.at(static_cast<size_t>(old.next_name)+(member-1)*4);c.variant=old.rotation;
  c.next_name=static_cast<int16_t>((old.next_name+1)%3);c.rotation=static_cast<int16_t>((old.rotation+(old.next_name==2))%3);
  c.active=1;c.health=100;c.rank=pilot_rank(c.total[0]);c.prior_skill=c.skill;return c;
}
}
campaign_debrief return_from_mission(career_record &career,hangar &roster,mission_results const &result,
  std::array<int16_t,100> const &counters,service_prices const &prices,random_state &random) {
  auto &state{career.states[0]};auto &inventory{career.inventories[0]};campaign_debrief debrief;
  for(auto [weapon,condition]:result.salvage)push(inventory,weapon,condition);
  size_t returned{};
  for(auto &c:state.crew)if(c.active&&c.hangar_slot!=-1){
    if(returned>=result.crew.size())throw std::runtime_error{"mission return lacks an active crew member"};
    auto const &r{result.crew[returned++]};auto h{roster.find(c.hangar_slot)};
    if(!h)throw std::runtime_error{"mission return crew refers to a missing HERC"};
    h->condition=r.condition;normalize_hangar_equipment(*h);c.health=h->condition.internal[9];c.current=r.statistics;
    for(size_t i{};i<3;++i){c.total[i]=wrap_word(c.total[i]+r.statistics[i]);debrief.statistics[i]=wrap_word(debrief.statistics[i]+r.statistics[i]);}
    c.rank=pilot_rank(c.total[0]);
  }
  if(returned!=result.crew.size())throw std::runtime_error{"mission return has unexpected crew"};
  state.counters=counters;state.counters[0]=result.grade;
  constexpr std::array<int16_t,7> awards{13,2,8,11,12,4,3};
  for(size_t i{};i<awards.size();++i){for(int j{};j<state.counters[8+i];++j)push(inventory,awards[i],100);state.counters[8+i]=0;}
  state.budget=wrap_long(int64_t{state.budget}+result.recovery);
  constexpr std::array<int16_t,13> restock{2,8,4,12,3,1,7,11,10,13,9,6,0};int added{};
  for(auto weapon:restock){if(added==5)break;if(state.technology.entries[8+weapon].available&&inventory[weapon].size()<2){push(inventory,weapon,100);++added;}}
  // The native cursor visits assigned crew in cyclic order and updates the
  // selected hangar slot. Budget is shared before dead wingmates are scrapped.
  auto const assigned{std::ranges::count_if(state.crew,[](auto const &c){return c.active&&c.hangar_slot!=-1;})};
  if(assigned){
    auto const share{std::bit_cast<int32_t>(static_cast<uint32_t>(state.budget)/static_cast<uint32_t>(assigned))};
    for(int i{};i<assigned;++i){auto const previous{state.crew_cursor};
      do{state.crew_cursor=static_cast<int16_t>((state.crew_cursor+1)%4);}while(!state.crew.at(static_cast<size_t>(state.crew_cursor)).active||state.crew[state.crew_cursor].hangar_slot==-1);
      auto &c{state.crew[state.crew_cursor]};if(state.crew_cursor!=previous)roster.selected=c.hangar_slot;c.budget_share=share;
    }
  }
  for(size_t i{};i<state.crew.size();++i){auto &c{state.crew[i]};
    if(!c.active||c.hangar_slot==-1||c.health)continue;
    ++debrief.deaths;if(i==static_cast<size_t>(state.selected_crew))continue;
    auto h{roster.find(c.hangar_slot)};if(!h)throw std::runtime_error{"dead wingmate has no chassis"};
    for(size_t j{};j<h->weapons.size();++j){auto const weapon{h->weapons[j]},condition{h->condition.equipment.at(j)};if(weapon==17)continue;
      if(condition<80)state.budget=wrap_long(int64_t{state.budget}+prices.weapons.at(static_cast<size_t>(weapon))*condition/100);
      else push(inventory,weapon,condition);
    }
    state.budget=wrap_long(int64_t{state.budget}+chassis_recovery(*h,prices));
    auto const slot{c.hangar_slot};std::erase_if(roster.hercs,[&](auto const &herc){return herc.slot==slot;});c=replacement(c,i,career.names,random);
  }
  state.missions=wrap_word(state.missions+1);advance_technology(state.technology,state.missions,state.mission_kind,result.grade);
  advance_construction(roster,prices);state.screen=1710;return debrief;
}
} // namespace earthsiege
