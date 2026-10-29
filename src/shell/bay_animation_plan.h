#pragma once
#include "shell/hangar_service.h"
#include "simulation/random_state.h"
namespace earthsiege {
struct bay_animation_state {
 std::array<int16_t,25> owners;
 std::array<int16_t,8> left{},right{};
 bay_animation_state(){owners.fill(-99);}
};
/// Native GO ownership of the 25 CFG/TM_FILES service-bay programs.
class bay_animation_plan {
 bay_animation_state current;
 void claim(int slot,size_t program);
public:
 bay_animation_plan()=default;
 explicit bay_animation_plan(bay_animation_state state):current{state}{}
 bay_animation_state const &state()const{return current;}
 void reset(hangar &roster,career_state &crew,bool campaign,int16_t mission_kind,service_prices const &,random_state &);
 void change(int16_t slot,int16_t role,hangar const &,service_prices const &,random_state &);
 bool active(size_t program,int16_t selected)const{return current.owners.at(program)==99||current.owners.at(program)==selected;}
};
}
