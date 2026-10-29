#pragma once
#include "shell/control_settings.h"
#include "shell/joystick_calibration.h"
#include "shell/save_store.h"
#include "render/control_settings_artwork.h"
#include "render/calibration_artwork.h"
#include "simulation/fixed_math.h"
namespace earthsiege {
std::array<joystick_calibration,2> load_host_calibrations(save_store const &);
class controls_dialog {
 preferences &prefs;save_store const &saves;
 std::array<joystick_calibration,2> &calibrations;
 control_settings_state settings;
 control_settings_artwork artwork;
 calibration_artwork calibration_view;
 bitmap backdrop;
 std::optional<joystick_calibration_state> calibration;
 std::array<joystick_hardware,2> hardware;
 int focus{},pressed{-1};bool reverse{},done_{},fire_held{};
 std::optional<vec2<int32_t>> warp;
 rectangle button(unsigned)const;
 unsigned count()const{return calibration?3:7;}
 void focus_button(int);
 void activate(unsigned,bool backward=false,bool joystick=false);
 void sample();
public:
 controls_dialog(resource_store const &,preferences &,save_store const &,std::array<joystick_calibration,2> &);
 bool done()const{return done_;}
 bool calibrating()const{return calibration.has_value();}
 void draw(framebuffer &)const;
 void key(uint16_t);
 void mouse_down(int x,int y,bool right);
 void mouse_up(int x,int y,bool right);
 void cancel_press(){pressed=-1;}
 void devices(std::array<joystick_hardware,2> const &);
 std::optional<vec2<int32_t>> take_warp();
};
}
