#pragma once
#include "shell/tactical_controller.h"
#include "shell/pointer_controls.h"
#include "simulation/mission_session.h"
#include "simulation/instrument_failure.h"
#include "resources/cockpit.h"

namespace earthsiege {
/// Separate original page clocks; a full restore occupies both VGA pages.
struct hdd_refresh_state {
  uint32_t map_deadline{30},damage_deadline{30};
  bool blink_phase{true};
  unsigned int restore_pages{2};
  void prepare() { restore_pages=2; }
  bool advance(bool map_page,uint32_t ticks) {
    bool const partial{restore_pages==0};
    if(restore_pages)--restore_pages;
    if(map_page&&!partial)return false;
    auto &deadline{map_page?map_deadline:damage_deadline};
    if(ticks<=deadline)return false;
    deadline=ticks+30;
    if(map_page)blink_phase=!blink_phase;
    return true;
  }
};

/// Lower cockpit MAP/DAMAGE pages and original single-wingmate commands.
class cockpit_hdd {
  random_state &random;
  instrument_failure_state failure;
  framebuffer cache;
  tactical_map map;
  full_damage_display damage;
  tactical_controller controller;
  tactical_map_state map_state;
  std::vector<size_t> wingmates;
  std::vector<bitmap> button_art;
  std::array<font,4> fonts; // green, yellow, red, blue
  interface_strings strings;
  std::array<std::string,3> pilot_names;
  std::array<std::string,9> weapon_label_pool;
  std::array<std::optional<size_t>,9> weapon_label_refs;
  rectangle bounds,map_bounds,command_bounds,list_bounds,painted,subject_light_bounds;
  std::array<rectangle,15> buttons;
  std::array<rectangle,3> selection_lights;
  std::array<rectangle,8> rows;
  std::array<size_t,15> button_ids;
  std::array<size_t,8> row_ids;
  size_t map_id;
  int direction_art{},selection_style{},page{},mode{},scroll{},subject{};
  int mouse_x{},mouse_y{},origin_x{},origin_y{};
  bool visible{},dirty{true},map_dirty{true},subject_light{true};
  std::optional<size_t> pending_button;
  void execute_button(size_t,mission_session &,pointer_controls &);
  void select_page(int value,pointer_controls &input);
  hdd_refresh_state refresh;
  unsigned int prompt_refreshes{};
  std::optional<size_t> selected_actor;
  std::optional<size_t> prompt;
  std::vector<uint16_t> sounds;
  std::optional<vec2<int32_t>> warp;
  void enable(pointer_controls &input);
  void select_wingmate(int slot,mission_session &session);
  void choose(int command,bool keyboard);
  void transmit(mission_session &session);
  void select_subject(int direction,mission_session const &session);
  void pick(mission_session &session,int32_t range);
  void paint(mission_session &session);
public:
  cockpit_hdd(resource_store const &resources,cockpit_layout const &layout,mission_session &session,pointer_controls &input,random_state &ui_random);
  bool command(uint16_t code,mission_session &session,pointer_controls &input,int32_t range);
  bool activate(control_activation const &action,mission_session &session,pointer_controls &input,int32_t range);
  void pointer(int x,int y);
  void update(mission_session &session,pointer_controls &input,camera_state const &camera,uint32_t ticks,int x,int y);
  void draw(framebuffer &frame,pointer_controls const &input,int x,int y)const;
  std::vector<uint16_t> take_sounds();
  std::optional<vec2<int32_t>> take_warp();
  tactical_control_state const &state()const;
  hdd_refresh_state const &refresh_state()const { return refresh; }
};
} // namespace earthsiege
