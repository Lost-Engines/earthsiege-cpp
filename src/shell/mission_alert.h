#pragma once
#include "render/dialog_widgets.h"
#include "resources/interface_strings.h"
#include "resources/resource_store.h"
#include <optional>
namespace earthsiege {
/// Original DBSIM 0x46efc nineteen-state general alert, shared by Pause,
/// Quit and the mission outcome notifications.
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
public:
  mission_alert(resource_store const &resources,int message);
  int message()const{return message_;}
  int count()const;
  bool exits_mission(int response)const;
  void draw(framebuffer &frame)const;
  std::optional<int> key(uint8_t scan);
  void mouse_down(int x,int y);
  std::optional<int> mouse_up(int x,int y);
};
}
