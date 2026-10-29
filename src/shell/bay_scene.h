#pragma once
#include "shell/bay_animation_plan.h"
#include "render/shell_animation_artwork.h"
#include "render/hangar_artwork.h"
namespace earthsiege {
/// Resident GO service-bay programs, preserving their order and retained page.
class bay_scene {
 struct entry {
  shell_animation player;
  shell_animation_artwork art;
  entry(resource_store const &r,std::string const &path):player{read_presentation(r.read("shell0","res32/"+path))},art{r}{}
 };
 std::vector<entry> programs;
 bay_animation_plan plan;
 framebuffer background,visible;
 void run(size_t index,int16_t elapsed,random_state &,shell_animation_sound *);
 void gate(int16_t selected,random_state &,shell_animation_sound *);
public:
 explicit bay_scene(resource_store const &resources);
 void reset(hangar &,career_state &,bool campaign,int16_t mission_kind,service_prices const &,random_state &,framebuffer const &base,shell_animation_sound *sound=nullptr);
 void redraw(hangar const &,hangar_artwork const &,armament_catalog const &,int selected_mount,framebuffer const &base,random_state &,shell_animation_sound *sound=nullptr);
 void construction(hangar const &,service_prices const &,random_state &);
 void advance(int16_t elapsed,random_state &,shell_animation_sound *sound=nullptr);
 framebuffer const &image()const{return visible;}
 framebuffer const &retained()const{return background;}
 bay_animation_plan const &assignments()const{return plan;}
 shell_animation_state const &state(size_t index)const{return programs.at(index).player.state();}
};
}
