#include "shell/mission_view.h"
#include "render/mfd_instruments.h"
#include "resources/input_tape.h"
#include "resources/name_table.h"
#include <filesystem>
#include <iostream>

int main(int argc,char **argv) try {
  using namespace earthsiege;
  if(argc!=3) {std::cerr<<"usage: capture_cockpit ASSET_DIRECTORY OUTPUT_DIRECTORY\n";return 1;}
  resource_store resources{argv[1]};std::filesystem::path const output{argv[2]};std::filesystem::create_directories(output);
  auto const names{read_resource_names(resources.read("simvol0","nam/MECHS.NAM"))};
  for(unsigned int demo{1};demo<=4;++demo) {
    auto const tape{read_input_tape(read_file(resources.root()/"TAPES"/("DEMO"+std::to_string(demo)+".TAP")))};
    auto const roster{read_player_roster(tape.starting_files[std::to_underlying(tape_file::player)])};
    auto const script{read_mission_script(tape.starting_files[std::to_underlying(tape_file::script)])};
    mission_session identity{resources,script,roster};
    auto const cockpit{read_cockpit(resources,names.at(static_cast<size_t>(identity.player_chassis())))};
    mfd_instruments controls{resources,cockpit.layout};auto const bounds{controls.button_bounds()};
    auto const origin{cockpit.windows[static_cast<size_t>(cockpit_view::forward)].instrument_origin};
    mission_view view{resources,demo};view.draw();view.image().save(output/("demo"+std::to_string(demo)+"-forward.ppm"),view.colours());
    view.change_cockpit_view(cockpit_view::forward);
    for(size_t page{};page<6;++page) {
      auto const &b{bounds[page]};view.mouse_position((b.left+b.right)/2-origin[0],(b.top+b.bottom)/2-origin[1]);
      view.mouse_button(false,true);view.mouse_button(false,false);view.step(false,false);view.draw();
      view.image().save(output/("demo"+std::to_string(demo)+"-page"+std::to_string(page)+".ppm"),view.colours());
    }
    std::cout<<"demo "<<demo<<": forward panel and all six MFD pages\n";
  }
  return 0;
} catch(std::exception const &error){std::cerr<<error.what()<<'\n';return 1;}
