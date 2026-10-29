#pragma once
#include "shell/dialog.h"
#include "shell/game_state.h"
#include "shell/save_store.h"
namespace earthsiege {
/// Application-level shell flow. Window input is translated before it reaches
/// this owner, so scripted tests exercise the same transitions as the UI.
class game_shell {
  resource_store const &resources;
  save_store saves;
  career_index careers;
  preferences preferences_;
  game_state game;
  std::unique_ptr<dialog> menu;
  std::vector<std::string> strings;
  font text_face;
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
  void mouse_down(int x,int y){menu->mouse_down(x,y);}
  void mouse_up(int x,int y);
  void cancel_press(){menu->cancel_press();}
  std::unique_ptr<mission_session> launch();
  void mission_return(mission_results const &result,mission_counters const &counters,int16_t return_seed=17);
  void draw(framebuffer &target)const;
  palette const &colours()const{return menu->current_palette();}
};
}
