#include "component_repair.h"
#include "simulation/fixed_math.h"
#include <stdexcept>
namespace earthsiege {
namespace {
constexpr std::array<int16_t,5> thresholds{91,75,52,1,0},next_band{100,91,75,52,1};
constexpr std::array<std::array<int,3>,6> groups{{{0,1,-1},{2,4,-1},{3,5,-1},{6,-1,-1},{7,9,11},{8,10,12}}};
}
int16_t component_condition(herc_condition const &c,repair_group group,int16_t index){
 auto const i{static_cast<size_t>(index)};
 if(group==repair_group::internal)return c.internal.at(i);
 if(group==repair_group::equipment)return c.equipment.at(i);
 int sum{},count{};for(auto part:groups.at(i))if(part>=0){sum+=c.armour.at(static_cast<size_t>(part));++count;}
 return static_cast<int16_t>(sum/count);
}
size_t component_count(player_herc const &h,repair_group group){
 if(group==repair_group::internal)return 9;
 if(group==repair_group::armour)return 6;
 return h.weapons.size();
}
component_repair::component_repair(player_herc const &h,career_state const &c,service_prices const &p)
 :prices{&p},original{h.condition},original_budget{c.budget}{select(h,repair_group::internal,0);}
int16_t component_repair::quote(player_herc const &h,bool increase,int16_t condition)const{
 auto const &chassis{prices->chassis.at(static_cast<size_t>(h.chassis))};auto const i{static_cast<size_t>(index_)};
 auto const price{group_==repair_group::internal?chassis.internal.at(i):group_==repair_group::armour?chassis.armour.at(i):prices->weapons.at(i)};
 auto const band{static_cast<size_t>(condition_tier(condition))};
 int difference{};
 if(increase)difference=(band?thresholds[band-1]:100)-condition;
 else difference=condition==100?100-thresholds[band]:condition-thresholds.at(band+1);
 return wrap_word(difference*price/100);
}
void component_repair::refresh(player_herc const &h){quote_=quote(h,true,component_condition(h.condition,group_,index_));}
void component_repair::select(player_herc const &h,repair_group group,int16_t index){
 if(index<0||static_cast<size_t>(index)>=component_count(h,group))throw std::out_of_range{"repair component"};
 group_=group;index_=index;tier_=condition_tier(component_condition(h.condition,group_,index_));refresh(h);
}
void component_repair::change_condition(player_herc &h,int16_t value)const{
 auto const i{static_cast<size_t>(index_)};
 if(group_==repair_group::internal)h.condition.internal.at(i)=value;
 else if(group_==repair_group::equipment)h.condition.equipment.at(i)=value;
 else for(auto part:groups.at(i))if(part>=0)h.condition.armour.at(static_cast<size_t>(part))=value;
}
void component_repair::spend(career_state &c,int32_t amount){
 c.budget=wrap_long(int64_t{c.budget}-amount);
 // Native indexing -1 writes over the crew-count/cursor header. Preserve the
 // transaction, but never reproduce that memory corruption for an unassigned HERC.
 if(c.crew_cursor>=0&&static_cast<size_t>(c.crew_cursor)<c.crew.size())c.crew[static_cast<size_t>(c.crew_cursor)].budget_share=wrap_long(int64_t{c.crew[static_cast<size_t>(c.crew_cursor)].budget_share}-amount);
}
bool component_repair::can_increase(career_state const &c)const{return static_cast<uint32_t>(c.budget)>=static_cast<uint32_t>(static_cast<int32_t>(quote_));}
void component_repair::increase(player_herc &h,career_state &c){
 if(!can_increase(c))return;
 allocated_=wrap_word(allocated_+quote_);change_condition(h,next_band.at(static_cast<size_t>(tier_)));
 if(tier_>0)--tier_;
 spend(c,quote_);refresh(h);
}
void component_repair::decrease(player_herc &h,career_state &c){
 auto const start{component_condition(original,group_,index_)},current{component_condition(h.condition,group_,index_)};
 if(current<=start)return;
 auto const initial_tier{condition_tier(start)};
 auto const refund{tier_==initial_tier||(tier_+1==initial_tier&&current<100)?quote(h,true,start):quote(h,false,current)};
 allocated_=wrap_word(allocated_-refund);spend(c,-refund);
 if(current<100)++tier_;
 change_condition(h,tier_==initial_tier?start:thresholds.at(static_cast<size_t>(tier_)));refresh(h);
}
void component_repair::accept(player_herc &h,career_state const &c)const{
 if(c.budget!=original_budget)change_condition(h,next_band.at(static_cast<size_t>(tier_)));
}
void component_repair::cancel(player_herc &h,career_state &c){spend(c,c.budget-original_budget);h.condition=original;}
}
