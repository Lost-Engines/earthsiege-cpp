#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include "resources/input_tape.h"
#include "resources/name_table.h"
#include "shell/cockpit_radio.h"

int main(int argc,char **argv)try{
  using earthsiege::camera_mode;
  using earthsiege::cockpit_radio;
  using earthsiege::cockpit_view;
  using earthsiege::framebuffer;
  using earthsiege::mission_session;
  using earthsiege::random_state;
  using earthsiege::read_cockpit;
  using earthsiege::read_file;
  using earthsiege::read_input_tape;
  using earthsiege::read_mission_script;
  using earthsiege::read_player_roster;
  using earthsiege::read_resource_names;
  using earthsiege::resource_store;
  using earthsiege::tape_file;if(argc != 2)return EXIT_FAILURE;resource_store resources{argv[1]};auto const names{read_resource_names(resources.read("simvol0","nam/MECHS.NAM"))};
  for(unsigned int demo{1};demo <= 4;++demo)for(bool const shown:{true,false})for(bool captions:{true,false}){
    auto const tape{read_input_tape(read_file(resources.root() / "TAPES" / ("DEMO" + std::to_string(demo) + ".TAP")))};
    mission_session session{resources,read_mission_script(tape.starting_files[std::to_underlying(tape_file::script)]),read_player_roster(tape.starting_files[std::to_underlying(tape_file::player)])};
    auto const cockpit{read_cockpit(resources,names.at(static_cast<size_t>(session.player_chassis())))};random_state random{17};cockpit_radio radio{resources,cockpit.layout,session,random};radio.captions(captions);
    auto camera{session.make_camera(shown?camera_mode::attached:camera_mode::orbit)};auto const origin{cockpit.windows[static_cast<size_t>(cockpit_view::lower)].instrument_origin};
    if(!session.transmit_order(4))throw std::runtime_error{"live scan order rejected"};
    bool visible{},starting{},finished{};size_t speech{},noise{};
    for(uint32_t ticks{};ticks < 600;++ticks){
      radio.update(session,camera,ticks,origin[0],origin[1]);auto const &state{radio.state()};starting |= state.starting;visible |= state.visible;if(starting && !state.active)finished = true;
      for(auto const &path:radio.take_speech()){
        bool found{};for(auto const &[name,volume]:resources.archives()){static_cast<void>(name);found |= volume.find(path) != nullptr;}
        if(!found)throw std::runtime_error{"unresolved live speech resource: " + path};
        ++speech;
      }
      noise += radio.take_sounds().size();framebuffer frame;radio.draw(frame);
    }
    if(!starting || visible != captions || !finished || speech != 1 || noise != 2)throw std::runtime_error{"live radio acknowledgement lifecycle"};
  }
  std::cout << "Four original demo rosters: visible and hidden acknowledgements, speech bindings and completion verified\n";return EXIT_SUCCESS;
}catch(std::exception const &e){std::cerr << e.what() << '\n';return EXIT_FAILURE;}
