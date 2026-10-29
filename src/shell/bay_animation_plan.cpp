#include "bay_animation_plan.h"
namespace earthsiege {
void bay_animation_plan::claim(int slot,size_t program){if(current.owners.at(program)==-99)current.owners[program]=static_cast<int16_t>(slot);}
void bay_animation_plan::change(int16_t slot,int16_t role,hangar const &roster,service_prices const &prices,random_state &random){
 if(slot<0||slot>=8)return;
 auto const i{static_cast<size_t>(slot)};
 if(role==0){claim(slot,6);current.left[i]=random.bounded(5);current.right[i]=2;}
 else if(role==2){
  claim(slot,i+9);current.left[i]=slot?random.bounded(5):static_cast<int16_t>(random.bounded(2)+3);
  auto h{roster.find(slot)};current.right[i]=random.bounded(!h||h->chassis<5?5:3);
 }else if(role==1){
  for(auto &owner:current.owners)if(owner==slot)owner=-99;
  auto const h{roster.find(roster.selected)};int progress{};
  if(h&&roster.find(roster.construction_slot)){
   int const duration{prices.chassis.at(static_cast<size_t>(h->chassis)).construction_missions};
   if(duration)progress=(duration-roster.construction_remaining)*100/duration;
  }
  if(!progress){claim(slot,8);current.left[i]=static_cast<int16_t>(random.bounded(2)+3);}
  else{
   claim(slot,7);
   if(progress<51&&h){constexpr std::array<size_t,8> weld{22,17,20,23,20,21,18,24};claim(slot,weld.at(static_cast<size_t>(h->chassis)));}
   current.left[i]=random.bounded(3);
  }
  current.right[i]=random.bounded(5);
 }
}
void bay_animation_plan::reset(hangar &roster,career_state &crew,bool campaign,int16_t mission_kind,service_prices const &prices,random_state &random){
 current.owners.fill(-99);for(size_t i{};i<4;++i)claim(99,i);
 if(mission_kind==4&&(!campaign||crew.current))claim(99,4);
 auto const pilot{crew.selected_crew>=0&&crew.selected_crew<4?crew.crew[static_cast<size_t>(crew.selected_crew)].hangar_slot:int16_t{-1}};
 for(size_t i{};i<roster.hercs.size();++i){
  for(int attempt{};attempt<roster.capacity;++attempt){
   roster.selected=static_cast<int16_t>((roster.selected+1)%roster.capacity);
   if(roster.find(roster.selected)&&roster.selected!=roster.construction_slot)break;
  }
  if(roster.find(roster.selected)&&roster.selected!=roster.construction_slot)change(roster.selected,roster.selected==pilot?0:2,roster,prices,random);
 }
 crew.crew_cursor=crew.selected_crew;if(crew.selected_crew!=-1)roster.selected=pilot;
}
}
