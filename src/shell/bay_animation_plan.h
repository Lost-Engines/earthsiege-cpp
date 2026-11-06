#pragma once
#include "shell/hangar_service.h"
#include "simulation/random_state.h"
namespace earthsiege {
struct bay_animation_state {
 std::vector<int16_t> owners;
 std::vector<int16_t> left,right;
 bay_animation_state(game_edition edition=game_edition::original):owners(edition==game_edition::expansion?26:25,-99),left(human_herc_count(edition)),right(human_herc_count(edition)){}
};
/// Native GO ownership of the 25 CFG/TM_FILES service-bay programs.
class bay_animation_plan {
 bay_animation_state current;
 void claim(int slot,size_t program);
public:
 explicit bay_animation_plan(game_edition edition=game_edition::original):current{edition}{}
 explicit bay_animation_plan(bay_animation_state state):current{state}{}
 bay_animation_state const &state()const{return current;}
 void reset(hangar &roster,career_state &crew,bool campaign,int16_t mission_kind,service_prices const &,random_state &);
 void change(int16_t slot,int16_t role,hangar const &,service_prices const &,random_state &);
 bool active(size_t program,int16_t selected)const{return current.owners.at(program)==99||current.owners.at(program)==selected;}
};
}
