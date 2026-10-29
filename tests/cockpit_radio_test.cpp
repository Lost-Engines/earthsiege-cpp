#include "shell/cockpit_radio.h"
#include "resources/input_tape.h"
#include "resources/name_table.h"
#include <iostream>
#include <stdexcept>

int main(int argc,char **argv)try{
  using namespace earthsiege;if(argc!=2)return 1;resource_store resources{argv[1]};auto const names{read_resource_names(resources.read("simvol0","nam/MECHS.NAM"))};
  for(unsigned int demo{1};demo<=4;++demo)for(bool const shown:{true,false}){
    auto const tape{read_input_tape(read_file(resources.root()/"TAPES"/("DEMO"+std::to_string(demo)+".TAP")))};
    mission_session session{resources,read_mission_script(tape.starting_files[std::to_underlying(tape_file::script)]),read_player_roster(tape.starting_files[std::to_underlying(tape_file::player)])};
    auto const cockpit{read_cockpit(resources,names.at(static_cast<size_t>(session.player_chassis())))};random_state random{17};cockpit_radio radio{resources,cockpit.layout,session,random};
    auto camera{session.make_camera(shown?camera_mode::attached:camera_mode::orbit)};auto const origin{cockpit.windows[static_cast<size_t>(cockpit_view::lower)].instrument_origin};
    if(!session.transmit_order(4))throw std::runtime_error{"live scan order rejected"};
    bool visible{},starting{},finished{};size_t speech{},noise{};
    for(uint32_t ticks{};ticks<600;++ticks){
      radio.update(session,camera,ticks,origin[0],origin[1]);auto const &state{radio.state()};starting|=state.starting;visible|=state.visible;if(visible && !state.active)finished=true;
      for(auto const &path:radio.take_speech()){
        bool found{};for(auto const &[name,volume]:resources.archives()){(void)name;found|=volume.find(path)!=nullptr;}
        if(!found)throw std::runtime_error{"unresolved live speech resource: "+path};
        ++speech;
      }
      noise+=radio.take_sounds().size();framebuffer frame;radio.draw(frame);
    }
    if(!starting || !visible || !finished || speech!=1 || noise!=2)throw std::runtime_error{"live radio acknowledgement lifecycle"};
  }
  std::cout<<"Four original demo rosters: visible and hidden acknowledgements, speech bindings and completion verified\n";return 0;
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return 1;}
