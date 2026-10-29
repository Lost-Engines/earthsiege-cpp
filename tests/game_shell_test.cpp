#include "shell/game_shell.h"
#include <chrono>
#include <algorithm>
#include <iostream>
#include <stdexcept>
using namespace earthsiege;
namespace {
void require(bool ok,char const *message){if(!ok)throw std::runtime_error{message};}
struct temporary_saves {
 std::filesystem::path path;
 explicit temporary_saves(std::filesystem::path const &root):path{root/("shell-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()))}{}
 ~temporary_saves(){std::error_code ignored;std::filesystem::remove_all(path,ignored);}
};
void click(game_shell &shell,uint16_t action){auto c{shell.current_dialog().find(action)};require(c&&c->enabled,"click target enabled");int x{(c->bounds.left+c->bounds.right)/2},y{(c->bounds.top+c->bounds.bottom)/2};shell.mouse_down(x,y);shell.mouse_up(x,y);}
void screen(game_shell const &shell,unsigned expected){if(shell.screen()!=expected){std::cerr<<"screen "<<shell.screen()<<" expected "<<expected<<'\n';throw std::runtime_error{"screen transition"};}framebuffer f;shell.draw(f);require(std::ranges::any_of(f.pixels(),[](uint8_t v){return v!=0;}),"screen draws");}
}
int main(int argc,char **argv)try{
 if(argc!=3)return 1;resource_store resources{argv[1]};temporary_saves root{argv[2]};game_shell shell{resources,root.path,17};
 screen(shell,1000);require(!shell.current_dialog().find(1700)->enabled,"no career to resume");
 click(shell,1100);screen(shell,1100);shell.key(80);require(shell.current_dialog().selection(1101)==1,"mission keyboard selection");
 click(shell,1110);screen(shell,1110);auto original{shell.settings().current()};click(shell,1111);require(shell.settings().current()[12]!=original[12],"single option changes");shell.key(1);screen(shell,1100);require(shell.settings().current()==original,"single cancel rolls back");
 click(shell,1501);screen(shell,1501);require(shell.state().ready()&&!shell.state().campaign(),"single mission prepared");click(shell,1800);screen(shell,1800);click(shell,100);screen(shell,1501);
 click(shell,100);require(shell.ready_to_launch(),"single launch request");auto single{shell.launch()};require(single!=nullptr&&!shell.ready_to_launch(),"launch consumed");shell.mission_return(single->results(),single->mission_variables());screen(shell,1510);
 click(shell,1000);click(shell,1200);screen(shell,1200);click(shell,1210);screen(shell,1210);shell.text_input("Test Pilot");click(shell,100);screen(shell,1700);require(shell.state().campaign(),"registered campaign");auto name{shell.state().mission_name()};
 click(shell,100);auto mission{shell.launch()};auto result{mission->results()};auto counters{mission->mission_variables()};result.grade=1;shell.mission_return(result,counters,13);screen(shell,1520);require(shell.state().crew().missions==1,"return advances career");
 click(shell,1710);screen(shell,1710);click(shell,1000);screen(shell,1000);
 game_shell reopened{resources,root.path,23};require(reopened.current_dialog().find(1700)->enabled,"resume survives restart");click(reopened,1700);screen(reopened,1710);require(reopened.state().crew().missions==1,"original save loaded");click(reopened,1499);screen(reopened,1700);require(reopened.state().crew().step==1,"next mission advances step");require(reopened.state().mission_name()!=name,"next mission differs");
 click(reopened,1000);click(reopened,1200);click(reopened,1220);screen(reopened,1220);click(reopened,1221);require(reopened.state().career().career.states[2].valid,"manual backup stored");
 click(reopened,1222);require(reopened.state().crew().step==1,"manual backup restored");
 std::cout<<"Single mission and career launch/return, isolated persistence, preferences and backups pass\n";return 0;
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return 1;}
