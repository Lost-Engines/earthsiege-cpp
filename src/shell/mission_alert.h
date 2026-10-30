#pragma once
#include "render/dialog_widgets.h"
#include "resources/interface_strings.h"
#include "resources/resource_store.h"
#include <optional>
#include "vectorstorm/vector/vector2.h"
#include "simulation/joystick_routing.h"
namespace earthsiege {
/// Original DBSIM 0x46efc nineteen-state general alert, shared by Pause,
/// Quit and the mission outcome notifications.
struct alert_input_result {bool consumed{};std::optional<int> response;};
class mission_alert {
  bitmap background;
  std::vector<bitmap> corners;
  std::array<button_style,4> styles;
  font title,body,button_metrics;
  std::array<font,3> button_fonts;
  interface_strings strings;
  int message_{};
  int focus_{},pressed_{-1};
  rectangle button(int i)const;
  std::optional<vec2<int32_t>> warp;
  bool fire_held{true},next_held{true};
  void focus(int);
public:
  mission_alert(resource_store const &resources,int message,int initial_focus=0);
  int message()const{return message_;}
  int count()const;
  bool exits_mission(int response)const;
  void draw(framebuffer &frame)const;
  int focused()const{return focus_;}
  alert_input_result input(uint16_t scan,bool fire,bool next,bool wcs=false);
  std::optional<int> key(uint16_t scan,bool wcs=false){return input(scan,false,false,wcs).response;}
  std::optional<int> devices(std::array<joystick_hardware,2> const &,bool wcs=false);
  std::optional<vec2<int32_t>> take_warp();
  void cancel_press(){pressed_=-1;}
  void mouse_down(int x,int y);
  std::optional<int> mouse_up(int x,int y);
};
}
