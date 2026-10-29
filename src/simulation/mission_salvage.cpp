#include "mission_salvage.h"
#include "simulation/fixed_math.h"
#include <array>
#include <stdexcept>
namespace earthsiege {
namespace {
int16_t percent(int32_t fraction){return wrap_word(((256-fraction)*100)>>8);}
int32_t fraction(int16_t amount,int16_t capacity){return amount<0?256:capacity?int32_t(amount)*256/capacity:0;}
int32_t multiply(int32_t amount,int16_t factor){auto const product{int64_t(amount)*factor};return wrap_long((product+(product<0?511:512))>>10);}
}
herc_condition returned_condition(damage_definition const &d,damage_state const &s){
  herc_condition out;
  for(size_t i{};i<out.armour.size();++i)out.armour[i]=percent(fraction(s.armour.at(i),d.components.at(i).armour));
  for(size_t i{};i<out.internal.size();++i)out.internal[i]=percent(d.hit_points.at(i)?fraction(s.internal.at(i),d.hit_points[i]):0);
  out.equipment.fill(100);
  for(size_t i{};i<9;++i){auto const a{d.components.at(19+i).armour},c{d.hit_points.at(12+i)};
    auto const accumulated{(s.armour.at(19+i)<0?a:s.armour[19+i])+(s.internal.at(12+i)<0?c:s.internal[12+i])};
    out.equipment[i]=percent(a+c?accumulated*256/(a+c):0);
  }
  return out;
}
salvaged_herc assess_salvage(damage_definition const &d,damage_state const &s,std::span<salvage_weapon const> weapons,int16_t hull_value,bool excluded){
  salvaged_herc out;if(excluded)return out;
  component_damage const damage{d,s};
  for(auto const &w:weapons){auto const f{damage.fraction(static_cast<size_t>(19+w.slot))};if(f<128)out.weapons.emplace_back(w.definition,percent(f));}
  constexpr std::array<int16_t,19> armor_weights{3000,3000,1000,1000,2000,2000,1000,2000,2000,1000,1000,500,500,2000,2000,1000,1000,500,500};
  constexpr std::array<int16_t,12> component_weights{1500,1500,0,0,1000,3000,1000,1000,1000,0,1500,1500};
  int32_t remainder{};
  for(size_t i{};i<armor_weights.size();++i){auto const amount{s.armour.at(i)},capacity{d.components.at(i).armour};if(s.present.at(i)&&amount>=0&&amount<150&&capacity)remainder=wrap_long(int64_t(remainder)+(capacity-amount)*armor_weights[i]/capacity);}
  for(size_t i{};i<component_weights.size();++i){auto const amount{s.internal.at(i)},capacity{d.hit_points.at(i)};if(capacity&&amount>=0&&amount<150)remainder=wrap_long(int64_t(remainder)+(capacity-amount)*component_weights[i]/capacity);}
  if(damage.fraction(0)==256)hull_value=static_cast<int16_t>(hull_value>>1);
  out.score=multiply(remainder,hull_value);return out;
}
int32_t recovery_value(int32_t score,int16_t mission_awards){return wrap_long(int64_t(multiply(score,2500))+int32_t(mission_awards)*25000);}
} // namespace earthsiege
