#include "hangar_service.h"
#include "simulation/fixed_math.h"
#include <algorithm>
#include <numeric>
#include <stdexcept>
namespace earthsiege {
namespace {
constexpr std::array<int16_t,5> targets{100,90,74,51,0},thresholds{91,75,52,1,0};
constexpr std::array<std::array<int,3>,6> groups{{{0,1,-1},{2,4,-1},{3,5,-1},{6,-1,-1},{7,9,11},{8,10,12}}};
int32_t q10(int32_t a,int32_t b){return static_cast<int32_t>((int64_t{a}*b+512)>>10);}
std::array<int16_t,6> armour_groups(herc_condition const &c) {
  std::array<int16_t,6> out{};
  for(size_t i{};i<groups.size();++i){int total{},count{};for(int part:groups[i])if(part>=0){total+=c.armour[part];++count;}out[i]=static_cast<int16_t>(total/count);}
  return out;
}
void service_condition(player_herc &herc,int16_t target) {
  herc.condition.armour.fill(target);std::fill_n(herc.condition.internal.begin(),9,target);
  for(size_t i{};i<herc.weapons.size();++i)herc.condition.equipment.at(i)=herc.weapons[i]==17?100:target;
}
}
service_prices read_service_prices(bytes source) {
  binary_reader file{source,"service price table"};auto const scale{file.integer<int16_t>()};
  std::array<int16_t,6> armour{};std::array<int16_t,10> internal{};
  for(auto &v:armour)v=file.integer<int16_t>();
  for(auto &v:internal)v=file.integer<int16_t>();
  auto const weapon_scale{file.integer<int16_t>()};service_prices prices;
  for(auto &v:prices.weapons)v=wrap_word(q10(file.integer<int16_t>(),weapon_scale));
  file.require_end();
  constexpr std::array<int32_t,8> purchase{50000,60000,80000,100000,120000,140000,170000,200000};
  constexpr std::array<int16_t,8> duration{1,1,2,2,2,3,3,3};
  for(size_t i{};i<purchase.size();++i){auto &c{prices.chassis[i]};c.purchase=purchase[i];c.construction_missions=duration[i];
    for(size_t j{};j<armour.size();++j)c.armour[j]=wrap_word(q10(q10(c.purchase,armour[j]),scale));
    for(size_t j{};j<internal.size();++j)c.internal[j]=wrap_word(q10(q10(c.purchase,internal[j]),scale));
  }
  return prices;
}
int16_t condition_tier(int16_t condition) {
  for(int16_t i{};i<5;++i)if(condition>=thresholds[i])return i;
  return 0;
}
int32_t repair_quote(player_herc const &herc,int16_t target,service_prices const &prices) {
  auto const &p{prices.chassis.at(static_cast<size_t>(herc.chassis))};int32_t sum{};
  auto contribution=[&](int16_t condition,int16_t price){if(condition<target)sum+=(targets[condition_tier(target)]-condition)*price/100;};
  auto const armour{armour_groups(herc.condition)};
  for(size_t i{};i<armour.size();++i)contribution(armour[i],p.armour[i]);
  for(size_t i{};i<9;++i)contribution(herc.condition.internal[i],p.internal[i]);
  for(size_t i{};i<herc.weapons.size();++i)if(herc.weapons[i]!=17&&herc.condition.equipment.at(i))contribution(herc.condition.equipment[i],prices.weapons.at(static_cast<size_t>(herc.weapons[i])));
  return sum;
}
repair_result repair_herc(player_herc &herc,int32_t budget,service_prices const &prices) {
  int total{std::accumulate(herc.condition.armour.begin(),herc.condition.armour.end(),0)};
  total=std::accumulate(herc.condition.internal.begin(),herc.condition.internal.begin()+9,total);
  total=std::accumulate(herc.condition.equipment.begin(),herc.condition.equipment.begin()+static_cast<ptrdiff_t>(herc.weapons.size()),total);
  auto const average{static_cast<int16_t>(total/static_cast<int>(22+herc.weapons.size()))};
  int16_t tier{};int32_t cost{repair_quote(herc,targets[tier],prices)};
  while(static_cast<uint32_t>(budget)<static_cast<uint32_t>(cost)&&tier<condition_tier(average))cost=repair_quote(herc,targets[++tier],prices);
  if(static_cast<uint32_t>(cost)>=static_cast<uint32_t>(budget))return {};
  service_condition(herc,targets[tier]);return {true,cost};
}
void apply_repair_policy(career_state &state,hangar &roster,service_prices const &prices) {
  int const start{state.repair_policy==2?0:state.repair_policy==1?1:4};
  for(int i{start};i<4;++i){auto &c{state.crew[i]};if(!c.active)continue;if(auto h{roster.find(c.hangar_slot)}) {
    auto const result{repair_herc(*h,c.budget_share,prices)};
    if(result.applied){state.budget=wrap_long(int64_t{state.budget}-result.cost);c.budget_share=wrap_long(int64_t{c.budget_share}-result.cost);}
  }}
}
void advance_construction(hangar &roster,service_prices const &prices) {
  if(roster.construction_slot<0)return;
  auto h{roster.find(roster.construction_slot)};if(!h)throw std::runtime_error{"construction slot has no chassis"};
  auto const &p{prices.chassis.at(static_cast<size_t>(h->chassis))};
  if(roster.construction_remaining>0)--roster.construction_remaining;
  if(roster.construction_remaining<1){service_condition(*h,100);roster.construction_slot=-1;roster.construction_value=0;return;}
  int const percent{(p.construction_missions-roster.construction_remaining)*100/p.construction_missions};
  int const price{p.purchase/1000};roster.construction_value=(price*80*percent/10000+(100-percent)*price)/100;
}
void discard_destroyed_equipment(player_herc &herc) {
  for(size_t i{};i<herc.weapons.size();++i)if(!herc.condition.equipment.at(i)){herc.weapons[i]=17;herc.condition.equipment[i]=100;}
}
void normalize_hangar_equipment(player_herc &herc) {
  discard_destroyed_equipment(herc);
  for(size_t i{};i<herc.weapons.size();++i)if(herc.weapons[i]>=9&&herc.weapons[i]<=11&&herc.weapon_values.at(i)==5)herc.weapon_values[i]=0;
}
int32_t chassis_recovery(player_herc const &herc,service_prices const &prices) {
  auto const &p{prices.chassis.at(static_cast<size_t>(herc.chassis))};auto const armour{armour_groups(herc.condition)};int32_t value{};
  for(size_t i{};i<armour.size();++i)value+=p.armour[i]*armour[i]/100;
  for(size_t i{};i<9;++i)value+=p.internal[i]*herc.condition.internal[i]/100;
  return value;
}

int16_t herc_average_condition(player_herc const &h){
 int sum{std::accumulate(h.condition.armour.begin(),h.condition.armour.end(),0)};
 sum=std::accumulate(h.condition.internal.begin(),h.condition.internal.begin()+9,sum);
 sum=std::accumulate(h.condition.equipment.begin(),h.condition.equipment.begin()+static_cast<ptrdiff_t>(h.weapons.size()),sum);
 return static_cast<int16_t>(sum/static_cast<int>(22+h.weapons.size()));
}
int16_t reserve_construction(hangar &h){
 if(h.construction_slot<0)for(int16_t i{};i<h.capacity;++i)if(!h.find(i)){h.construction_slot=i;break;}
 h.selected=h.construction_slot;return h.selected;
}
bool construct_herc(hangar &h,career_state &c,int16_t chassis,size_t mounts,service_prices const &prices){
 auto const &p{prices.chassis.at(static_cast<size_t>(chassis))};
 if(h.selected<0||h.find(h.selected)||c.budget<p.purchase||mounts>10)return false;
 player_herc model;model.chassis=chassis;model.slot=h.selected;model.extra=2;
 model.weapons.assign(mounts,17);model.weapon_values.assign(mounts,0);
 model.condition.armour.fill(100);model.condition.internal.fill(100);model.condition.internal[9]=0;model.condition.equipment.fill(100);
 h.hercs.push_back(std::move(model));h.construction_remaining=p.construction_missions;h.construction_value=p.purchase/1000;c.budget-=p.purchase;return true;
}
bool scrap_selected(hangar &h,career_state &c,armory &inventory,service_prices const &prices){
 auto model{h.find(h.selected)};if(!model)return false;
 auto const slot{h.selected};bool const building{slot==h.construction_slot};
 for(size_t i{};i<model->weapons.size();++i){auto const weapon{model->weapons[i]},condition{model->condition.equipment.at(i)};if(weapon==17)continue;
  if(condition<80)c.budget=wrap_long(int64_t{c.budget}+prices.weapons.at(static_cast<size_t>(weapon))*condition/100);
  else{auto &stock{inventory.at(static_cast<size_t>(weapon))};stock.insert(stock.begin(),condition);}
 }
 auto const value{building&&h.construction_value?h.construction_value*1000:chassis_recovery(*model,prices)};
 c.budget=wrap_long(int64_t{c.budget}+value);
 if(building){h.construction_remaining=0;h.construction_value=0;}
 else for(auto &member:c.crew)if(member.hangar_slot==slot){member.hangar_slot=-1;break;}
 std::erase_if(h.hercs,[&](auto const &entry){return entry.slot==slot;});
 if(!building)for(int i{};i<h.capacity;++i){h.selected=static_cast<int16_t>((h.selected+1)%h.capacity);if(h.find(h.selected)&&h.selected!=h.construction_slot)break;if(h.selected==slot)break;}
 c.crew_cursor=-1;for(size_t i{};i<c.crew.size();++i)if(c.crew[i].hangar_slot==h.selected){c.crew_cursor=static_cast<int16_t>(i);break;}
 return true;
}

} // namespace earthsiege
