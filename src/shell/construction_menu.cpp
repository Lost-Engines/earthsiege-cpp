#include "construction_menu.h"
namespace earthsiege {
construction_menu::construction_menu(hangar &h,career_state const &c,service_prices const &p,int16_t last)
 :chassis_{last},prices{&p}{
 reserve_construction(h);
 if(auto model{h.find(h.construction_slot)}){chassis_=model->chassis;enabled_=true;cycling_=false;}
 else{cycling_=true;enabled_=h.selected>=0&&static_cast<uint32_t>(c.budget)>=static_cast<uint32_t>(p.chassis.at(static_cast<size_t>(chassis_)).purchase);}
}
void construction_menu::cycle(career_state const &c,int direction){
 if(!cycling_)return;
 for(int i{};i<8;++i){
  chassis_=static_cast<int16_t>((chassis_+(direction>0?1:7))%8);
  if(c.technology.entries.at(static_cast<size_t>(chassis_)).available)break;
 }
 enabled_=static_cast<uint32_t>(c.budget)>=static_cast<uint32_t>(prices->chassis.at(static_cast<size_t>(chassis_)).purchase);
}
void construction_menu::activate(hangar &h,career_state &c,armory &inventory,armament_catalog const &catalog){
 if(!enabled_)return;
 if(building(h)){
  auto const cursor{c.crew_cursor};scrap_selected(h,c,inventory,*prices);c.crew_cursor=cursor;cycling_=true;
  // Original 0x30c93 re-enables cycling, but does not recompute affordability.
 }else if(construct_herc(h,c,chassis_,catalog.mounts(chassis_).size(),*prices))cycling_=false;
}
}
