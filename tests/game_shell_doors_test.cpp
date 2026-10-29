#include "shell/game_shell.h"
#include <algorithm>
#include <chrono>
#include <iostream>
using namespace earthsiege;
namespace {
void require(bool value,char const *message){if(!value)throw std::runtime_error{message};}
struct directory{
 std::filesystem::path path;
 explicit directory(std::filesystem::path const &root):path{root/("doors-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()))}{}
 ~directory(){std::error_code error;std::filesystem::remove_all(path,error);}
};
void doors(game_shell &shell){unsigned ticks{};while(shell.transition_active()&&ticks++<40)shell.advance(1);require(!shell.transition_active(),"door transition finishes");}
void snapshot(game_shell &shell,std::filesystem::path const &root,char const *name){framebuffer f;shell.draw(f);f.save(root/name,shell.colours());}
}
int main(int argc,char **argv)try{
 if(argc!=3)return 1;
 resource_store resources{argv[1]};directory root{argv[2]};preferences prefs;prefs.set(0,0);prefs.set(1,1);prefs.set(2,0);prefs.commit();save_store{root.path}.save_preferences(prefs);
 game_shell shell{resources,root.path};
 shell.startup(false);require(shell.transition_active(),"skipping intro retains menu doors");
 shell.advance(6);snapshot(shell,argv[2],"menu-closing.ppm");doors(shell);shell.take_audio();
 shell.action(1100);shell.action(1501);require(shell.transition_active(),"mission briefing opens doors");
 shell.key(1);shell.mouse_down(300,180);shell.mouse_up(300,180);require(shell.transition_active()&&!shell.ready_to_launch(),"transition ignores and consumes input");
 shell.advance(6);snapshot(shell,argv[2],"menu-opening.ppm");doors(shell);
 auto audio{shell.take_audio()};require(std::ranges::any_of(audio,[](int16_t value){return value!=0;}),"door effect plays with music and speech disabled");
 require(shell.presentation_active()&&shell.screen()==1499,"door completion releases commander portrait");
 shell.key(1);require(shell.screen()==1501&&!shell.presentation_active(),"skip reaches briefing");
 shell.action(100);shell.key(1);require(shell.ready_to_launch(),"single launch after departure");
 auto mission{shell.launch()};auto result{mission->results()};result.crew[0].condition.internal[9]=0;
 shell.mission_return(result,mission->mission_variables());require(shell.screen()==1100&&!shell.presentation_active(),"single pilot death returns to mission selection without a career ending");
 shell.action(1300);shell.action(1302);require(shell.presentation_active()&&shell.screen()==0,"credits command starts original movie");
 shell.advance(360);shell.take_audio();snapshot(shell,argv[2],"credits.ppm");shell.key(1);require(shell.presentation_active(),"credits have a second page");
 shell.key(1);require(shell.transition_active(),"credits return closes menu doors");doors(shell);
 require(shell.screen()==1300&&!shell.presentation_active(),"credits return to options");
 std::cout<<"Menu doors, transition input, independent sound, credits and single-mission death return pass\n";return 0;
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return 1;}
