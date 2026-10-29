#include "shell/game_shell.h"
#include <algorithm>
#include <chrono>
#include <iostream>
#include <stdexcept>
using namespace earthsiege;
namespace {
void require(bool value,char const *message){if(!value)throw std::runtime_error{message};}
struct temporary_saves{
 std::filesystem::path path;
 explicit temporary_saves(std::filesystem::path const &parent):path{parent/("speech-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()))}{}
 ~temporary_saves(){std::error_code ignored;std::filesystem::remove_all(path,ignored);}
};
void click(game_shell &shell,uint16_t id){
 auto item{shell.current_dialog().find(id)};require(item&&item->enabled,"enabled speech-flow control");
 int x{(item->bounds.left+item->bounds.right)/2},y{(item->bounds.top+item->bounds.bottom)/2};
 shell.mouse_down(x,y);shell.mouse_up(x,y);
}
unsigned finish(game_shell &shell){
 unsigned ticks{};
 while(shell.presentation_active()&&ticks<120*300){shell.advance(2);ticks+=2;shell.take_audio();}
 require(!shell.presentation_active(),"presentation finishes without input");return ticks;
}
void snapshot(game_shell const &shell,std::filesystem::path const &root,char const *name){
 framebuffer f;shell.draw(f);f.save(root/name,shell.colours());
}
}
int main(int argc,char **argv)try{
 if(argc!=3)return 1;
 resource_store resources{argv[1]};temporary_saves root{argv[2]};
 preferences prefs;prefs.set(0,0);prefs.set(1,0);prefs.set(2,2);prefs.commit();
 save_store{root.path}.save_preferences(prefs);
 game_shell shell{resources,root.path,17};
 click(shell,1100);click(shell,1501);
 require(shell.presentation_active()&&shell.screen()==1499,"single mission opens commander speech");
 framebuffer first;shell.draw(first);bool movement{},voice{};
 for(unsigned t{};t<180;++t){
  shell.advance(2);framebuffer now;shell.draw(now);movement|=first.pixels()!=now.pixels();
  auto pcm{shell.take_audio()};voice|=std::ranges::any_of(pcm,[](int16_t s){return s!=0;});
 }
 require(movement,"commander portrait animates");
 require(voice,"speech plays independently of disabled music and effects");
 snapshot(shell,argv[2],"commander-speech.ppm");
 finish(shell);require(shell.screen()==1501,"commander finishes into briefing map");
 click(shell,100);
 require(shell.presentation_active()&&!shell.ready_to_launch(),"departure speech precedes launch");
 snapshot(shell,argv[2],"mechanic-speech.ppm");
 shell.key(1);require(shell.ready_to_launch(),"skipping departure releases launch");
 auto mission{shell.launch()};shell.mission_return(mission->results(),mission->mission_variables());
 require(shell.presentation_active(),"single return speaks before debrief");
 finish(shell);require(shell.screen()==1510,"return speech finishes into debrief");
 click(shell,1000);click(shell,1200);click(shell,1210);shell.text_input("Speech Pilot");click(shell,100);
 require(shell.presentation_active()&&shell.screen()==1490,"new career introduces its chapter");
 shell.advance(60);shell.take_audio();snapshot(shell,argv[2],"general-speech.ppm");
 auto ticks{finish(shell)};
 require(ticks>120*10&&shell.screen()==1700,"general plays multiple clips before service bay");
 click(shell,1500);require(shell.presentation_active(),"campaign briefing uses commander");
 // A click to dismiss speech must not reach a control in the newly exposed menu.
 shell.mouse_down(300,180);shell.mouse_up(300,180);
 require(!shell.presentation_active()&&shell.screen()==1500&&!shell.ready_to_launch(),"speech skip consumes mouse release");
 click(shell,100);require(shell.presentation_active(),"campaign departure speaks");
 finish(shell);require(shell.ready_to_launch(),"natural departure completion releases launch");
 std::cout<<"Live commander, general, departure and debrief speech, voice preferences, completion and input handoff pass\n";
 return 0;
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return 1;}
