#pragma once
#include "shell/dialog.h"
#include "audio/shell_audio.h"
#include "shell/bay_scene.h"
#include "render/weapon_preview.h"
#include "render/briefing_map.h"
#include "shell/game_state.h"
#include "shell/save_store.h"
#include "render/hangar_artwork.h"
#include "render/crew_artwork.h"
#include "shell/component_repair.h"
#include "render/construction_artwork.h"
namespace earthsiege {
/// Application-level shell flow. Window input is translated before it reaches
/// this owner, so scripted tests exercise the same transitions as the UI.
class game_shell {
  resource_store const &resources;
  save_store saves;
  career_index careers;
  preferences preferences_;
  shell_audio audio;
  game_state game;
  std::unique_ptr<dialog> menu;
  std::unique_ptr<briefing_map> briefing;
  std::unique_ptr<dialog> warning_parent;
  framebuffer warning_frame;
  void warn(unsigned id);
  void dismiss_warning();
  void next_mission();
  std::vector<std::string> strings;
  font text_face;
  service_prices prices;
  std::optional<component_repair> repair_edit;
  void refresh_repair();
  construction_artwork build_view;
  std::optional<construction_menu> build_menu;
  int16_t build_chassis{};
  void refresh_construction();
  armament_catalog armament;
  hangar_artwork bay;
  std::unique_ptr<bay_scene> bay_motion;
  framebuffer bay_backdrop;
  std::vector<uint8_t> displayed_hangar;
  int displayed_mount{-2};
  uint64_t animation_ticks{};
  void refresh_bay();
  void action_impl(uint16_t id);
  crew_artwork crew_view;
  std::optional<crew_assignment> crew_edit;
  int selected_mount{-1};
  std::optional<int> pressed_mount;
  bool missile_menu{};
  std::unique_ptr<weapon_preview> weapon_motion;
  int preview_index{-2};
  void refresh_preview();
  std::vector<int16_t> weapon_choices;
  void refresh_armament(std::optional<int16_t> wanted={});
  void choose_armament();
  int single_kind{},battle_set{};
  unsigned int options_owner{1000},last_screen{1000};
  int text_scroll{};
  std::vector<size_t> history_slots;
  bool quitting{},launch_requested{};
  void show(unsigned int id);
  void refresh_values();
  void save_career();
  void select_career();
  void register_career();
  void begin();
  void draw_text(framebuffer &frame,std::string const &text,rectangle const &bounds,int scroll=0)const;
public:
  game_shell(resource_store const &resources,std::filesystem::path save_directory,int seed=17);
  unsigned int screen()const{return menu->id();}
  bool quit_requested()const{return quitting;}
  bool ready_to_launch()const{return launch_requested;}
  preferences const &settings()const{return preferences_;}
  dialog const &current_dialog()const{return *menu;}
  game_state const &state()const{return game;}
  void action(uint16_t id);
  void key(uint8_t scan);
  void text_input(std::string_view text){menu->text_input(text);}
  void mouse_move(int x,int y);
  void mouse_down(int x,int y);
  void mouse_up(int x,int y);
  void cancel_press(){pressed_mount.reset();menu->cancel_press();}
  std::unique_ptr<mission_session> launch();
  void mission_return(mission_results const &result,mission_counters const &counters,int16_t return_seed=17);
  void advance(unsigned timer_ticks=3);
  std::vector<int16_t> take_audio(){return audio.take_output();}
  void draw(framebuffer &target)const;
  palette const &colours()const{return menu->current_palette();}
};
}
