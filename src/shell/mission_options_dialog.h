#pragma once
#include "shell/mission_preferences.h"
#include "shell/controls_dialog.h"
#include "render/mission_preferences_artwork.h"
namespace earthsiege {
/// PRF_ALRT modal ownership, shared by host input and recorded input.
class mission_options_dialog {
 resource_store const &resources;
 preferences &prefs;
 save_store const &saves;
 std::array<joystick_calibration,2> &calibrations;
 mission_preferences settings;
 mission_preferences_artwork artwork;
 std::unique_ptr<controls_dialog> controls;
 int focus{},pressed{-1};bool reverse{},done_{},fire_held{},next_held{};
 std::optional<vec2<int32_t>> warp;
 void focus_button(int);
 void activate(unsigned,bool backward=false);
 void finish_child();
 std::function<void()> click;
public:
 mission_options_dialog(resource_store const &,preferences &,save_store const &,std::array<joystick_calibration,2> &);
 void feedback(std::function<void()> callback){click=std::move(callback);}
 bool done()const{return done_;}
 bool child_active()const{return controls!=nullptr;}
 int focused()const{return focus;}
 void key(uint16_t);
 void mouse_down(int x,int y,bool right=false);
 void mouse_up(int x,int y,bool right=false);
 void devices(std::array<joystick_hardware,2> const &);
 void cancel_press();
 std::optional<vec2<int32_t>> take_warp();
 void draw(framebuffer &)const;
};
}
