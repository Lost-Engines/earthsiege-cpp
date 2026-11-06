#include "shell/game_shell.h"
#include "shell/demo_playback.h"
#include <chrono>
#include <iostream>
#include <stdexcept>
using namespace earthsiege;
namespace {
void require(bool ok,char const *why){if(!ok)throw std::runtime_error{why};}
struct cleanup{std::filesystem::path path;~cleanup(){std::error_code ec;std::filesystem::remove_all(path,ec);}};
void click(game_shell &shell,uint16_t id){
  auto const *control{shell.current_dialog().find(id)};
  require(control&&control->enabled,"enabled menu control");
  int const x{(control->bounds.left+control->bounds.right)/2},y{(control->bounds.top+control->bounds.bottom)/2};
  shell.mouse_down(x,y);shell.mouse_up(x,y);
  for(unsigned i{};shell.transition_active()&&i<100;++i)shell.advance(20);
}
}
int main(int argc,char **argv)try{
  if(argc<3)return 1;
  resource_store resources{argv[1]};
  cleanup temp{std::filesystem::path{argv[2]}/("options-demo-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()))};
  game_shell shell{resources,temp.path};
  click(shell,1300);click(shell,1310);
  require(shell.current_dialog().selection(1316)==3,"255 detail values display MAX, not HIGH");
  auto const initial{shell.settings().current()};
  // GO's four stored presets, followed through the actual mouse button.
  for(int level{};level!=4;++level){
    click(shell,1313);
    require(shell.current_dialog().selection(1316)==level,"all four detail presets are reachable");
    auto const &v{shell.settings().current()};
    require(v[3]==(level!=0)&&v[4]==63+64*level&&v[5]==v[4]&&v[6]==v[4],"original preset bytes");
  }
  click(shell,1313);click(shell,99);click(shell,1310);
  require(shell.settings().current()==initial&&shell.current_dialog().selection(1316)==3,"Cancel restores values and label");
  click(shell,1313);click(shell,100);click(shell,1310);
  require(shell.current_dialog().selection(1316)==0,"accepted preset retained when reopening");
  click(shell,1313);click(shell,100);click(shell,1310);
  require(shell.current_dialog().selection(1316)==1,"MEDIUM retains its separate shell selector");
  // Put the original maximum values back for replay and persistence checks.
  click(shell,1313);click(shell,1313);click(shell,100);
  game_shell restored{resources,temp.path};click(restored,1300);click(restored,1310);
  require(restored.current_dialog().selection(1316)==3,"MAX survives restart");
  click(shell,1301);
  require(shell.take_demo_request()&&!shell.take_demo_request()&&!shell.ready_to_launch(),"View Demo requests playback once, not a career launch");
  shell.demo_return();require(shell.screen()==1000,"demo returns to main menu");
  auto const full{argc>3};
  for(unsigned demo{argc>4?static_cast<unsigned>(std::stoul(argv[4])):1};demo<=4;++demo){
    demo_playback playback{resources,demo,shell.settings(),temp.path};
    size_t shown{};
    while(!playback.done()){
      playback.step([&](unsigned,framebuffer const &frame){
        playback.view().take_audio();playback.view().take_pan_frames();
        require(frame.pixels().size()==320*200,"demo presents game frames");
        ++shown;return full||playback.frames_played()<3;
      });
      require(shown<20000,"playback terminates");
    }
    std::cout<<"Completed demo "<<demo<<": "<<playback.frames_played()<<" frames, "<<playback.modal_packets_read()<<" modal packets, "<<playback.packets_read()<<'/'<<playback.packet_count()<<std::endl;
    if(full){require(playback.packets_read()==playback.packet_count(),"complete recording consumed");require(playback.frames_played()>1000,"full demo simulates");}
    else require(playback.frames_played()==3&&playback.packets_read()<playback.packet_count(),"host input can interrupt playback");
  }
  require(save_store{temp.path,resources.edition()}.index().entries.empty(),"playback creates no career");
  std::cout<<"Options presets, persistence, demo launch, playback and return pass\n";
  return 0;
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return 1;}
