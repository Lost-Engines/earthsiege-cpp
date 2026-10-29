#pragma once
#include "shell/hangar_service.h"
#include "shell/armament.h"
namespace earthsiege {
class construction_menu {
 int16_t chassis_{};
 bool enabled_{},cycling_{};
 service_prices const *prices;
public:
 construction_menu(hangar &,career_state const &,service_prices const &,int16_t last=0);
 void cycle(career_state const &,int direction);
 void activate(hangar &,career_state &,armory &,armament_catalog const &);
 bool building(hangar const &h)const{return h.find(h.construction_slot)!=nullptr;}
 int16_t chassis()const{return chassis_;}
 bool enabled()const{return enabled_;}
 bool cycling()const{return cycling_;}
};
}
