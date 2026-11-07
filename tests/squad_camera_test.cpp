#include "shell/mission_view.h"
#include <algorithm>
#include <iostream>
using namespace earthsiege;
namespace{void require(bool ok,char const *why){if(!ok)throw std::runtime_error{why};}}
int main(int argc,char **argv)try{
 if(argc!=2)return 1;
 resource_store resources{argv[1]};size_t switches{};
 for(unsigned demo{1};demo<=4;++demo){
  mission_view view{resources,demo};auto const player{view.simulation().player_id()};
  require(!view.simulation().objective_text.empty(),"demo restores its objectives");
  auto step=[&](uint16_t code){tape_frame f;f.delta=1;f.command=code;require(view.step_recorded(f,0),"camera command keeps mission running");};
  step(49);require(view.camera_actor()==player,"N ignored in cockpit");
  view.change_camera(camera_mode::orbit);
  tape_frame turn;turn.delta=1;turn.primary_axes.x=256;
  require(view.step_recorded(turn,0),"camera turn keeps mission running");step(0);auto const original_orbit{view.view_camera().orbit_angles};
  require(original_orbit.z!=0,"saved player camera has a distinct orbit");
  auto actors{view.simulation().hercs()};size_t current{player};
  for(size_t iteration{};iteration<=actors.size();++iteration){
   size_t expected{current};
   for(size_t offset{1};offset<actors.size();++offset){auto next{(current+offset)%actors.size()};if(actors[next].group==actors[player].group&&actors[next].actor.body().movement().position.z>=0){expected=next;break;}}
   step(49);require(view.camera_actor()==expected,"N follows original actor order within player group");current=expected;++switches;
   auto reference{view.view_camera()};update_camera(reference,view.simulation().view_target(current));
   require(reference.position==view.view_camera().position,"camera follows selected squad HERC");
   if(current==player){require(view.view_camera().orbit_angles==original_orbit,"player orbit restored after squad cycle");break;}
   require(view.view_camera().orbit_angles==vec3<uint16_t>{},"newly bound squad camera starts at zero orbit");
  }
  step(49);view.change_camera(camera_mode::attached);require(view.camera_actor()==player,"returning to cockpit restores player target");
 }
 require(switches>=4,"squad cycle exercised");std::cout<<switches<<" squad camera switches and four restored objective lists pass\n";
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return 1;}
