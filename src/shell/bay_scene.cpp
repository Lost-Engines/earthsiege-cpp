#include "bay_scene.h"
#include <stdexcept>
namespace earthsiege {
bay_scene::bay_scene(resource_store const &r):plan{r.edition()}{
 auto data{r.read("shell0","cfg/tm_files.dat")};binary_reader f{data};
 auto groups{f.integer<uint8_t>()};
 for(unsigned group{};group<groups;++group){
  auto count{f.integer<uint8_t>()};
  if(!count){if(!f.cstring().empty())f.fail("invalid empty animation group");continue;}
  for(unsigned i{};i<count;++i){auto path{f.cstring()};if(group==3)programs.emplace_back(r,path);}
 }
 f.require_end();if(programs.size()!=plan.state().owners.size())throw std::runtime_error{"invalid service bay animation group"};
}
void bay_scene::run(size_t i,int16_t elapsed,random_state &random,shell_animation_sound *sound){
 auto &p{programs.at(i)};p.player.advance(elapsed,random,sound);p.art.apply(p.player.commands(),visible,background);
}
void bay_scene::gate(int16_t selected,random_state &random,shell_animation_sound *sound){
 for(size_t i{};i<programs.size();++i){programs[i].player.mode(plan.active(i,selected)?0:2);run(i,0,random,sound);}
}
void bay_scene::reset(hangar &h,career_state &c,bool campaign,int16_t kind,service_prices const &prices,random_state &random,framebuffer const &base,shell_animation_sound *sound){
 background=visible=base;plan.reset(h,c,campaign,kind,prices,random);gate(h.selected,random,sound);
}
void bay_scene::redraw(hangar const &h,hangar_artwork const &art,armament_catalog const &catalog,int mount,framebuffer const &base,random_state &random,shell_animation_sound *sound){
 background=visible=base;
 if(h.selected>=0&&static_cast<size_t>(h.selected)<plan.state().left.size()){
  auto slot{static_cast<size_t>(h.selected)};
  programs[0].player.select_page(static_cast<uint16_t>(plan.state().left[slot]+1));
  programs[1].player.select_page(static_cast<uint16_t>(plan.state().right[slot]+1));
  programs[2].player.select_page(static_cast<uint16_t>(slot+1));
  if(auto herc{h.find(h.selected)}){
   auto const remaining{h.selected==h.construction_slot?h.construction_remaining:0};
   art.body(background,*herc,remaining);art.body(visible,*herc,remaining);
   run(1,0,random,sound);
   art.equipment(background,*herc,catalog,mount);art.equipment(visible,*herc,catalog,mount);
   run(0,0,random,sound);run(2,0,random,sound);
  }
 }
 gate(h.selected,random,sound);
}
void bay_scene::construction(hangar const &h,service_prices const &prices,random_state &random){plan.change(h.construction_slot,1,h,prices,random);}
void bay_scene::speech_mode(bool active){
 for(auto index:{3U,6U})programs[index].player.mode(active?2:0);
}
void bay_scene::compose(framebuffer &front,framebuffer &back,int16_t elapsed,random_state &random,shell_animation_sound *sound){
 std::swap(visible,front);std::swap(background,back);
 advance(elapsed,random,sound);
 std::swap(visible,front);std::swap(background,back);
}
void bay_scene::advance(int16_t elapsed,random_state &random,shell_animation_sound *sound){
 visible=background;
 for(size_t i{};i<programs.size();++i)run(i,elapsed,random,sound);
}
}
