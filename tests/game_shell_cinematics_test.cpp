#include "shell/game_shell.h"
#include <chrono>
#include <iostream>
using namespace earthsiege;
namespace {
void require(bool v,char const *message){if(!v)throw std::runtime_error{message};}
struct directory {
 std::filesystem::path path;
 explicit directory(std::filesystem::path const &root):path{root/("cinema-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()))}{}
 ~directory(){std::error_code ignored;std::filesystem::remove_all(path,ignored);}
};
void skip(game_shell &shell){
 for(unsigned i{};shell.presentation_active()&&i<64;++i)shell.key(1);
 require(!shell.presentation_active(),"presentation skip chain terminates");
}
void frame(game_shell &shell,std::filesystem::path const &root,std::string const &name){
 shell.advance(240);shell.take_audio();framebuffer f;shell.draw(f);f.save(root/name,shell.colours());
}
}
int main(int argc,char **argv)try{
 if(argc!=3)return 1;resource_store r{argv[1]};directory root{argv[2]};save_store store{root.path};
 preferences prefs;prefs.set(2,2);prefs.commit();store.save_preferences(prefs);
 game_shell opening{r,root.path,17};opening.startup();
 require(opening.presentation_active(),"startup starts original cinematics");frame(opening,argv[2],"opening-cinematic.ppm");
 unsigned ticks{};
 while(opening.presentation_active()&&ticks<120*600){opening.advance(2);opening.take_audio();ticks+=2;}
 require(!opening.presentation_active()&&opening.screen()==1000,"complete opening reaches main menu");
 opening.startup();opening.key(1);
 require(opening.presentation_active(),"skipping publisher logo still shows story intro");
 opening.key(1);require(opening.presentation_active(),"skipping story still shows title card");
 opening.key(1);require(!opening.presentation_active(),"title skip returns to menu");
 game_state seed{r,17};seed.new_campaign();
 career_entry entry{"Cinema Pilot",0,"car/test.car","car/test.mec"};store.save_index({0,{entry}});
 auto const graph{read_campaign_table(r.read("zones","bst/btlset_0.dat"))};
 for(int scenario{};scenario<5;++scenario){
  auto career{seed.career()};auto &c{career.career.states[0]};
  c.node=scenario==1||scenario==2?7:0;c.step=(scenario==1||scenario==2||scenario==4)?static_cast<int16_t>(graph.nodes[c.node].size()-1):0;
  c.screen=1700;store.save(entry,career);
  game_shell shell{r,root.path,17};shell.action(1700);shell.action(100);skip(shell);
  require(shell.ready_to_launch(),"prepared campaign launches");
  auto mission{shell.launch()};auto result{mission->results()};result.grade=scenario==2?7:1;result.recovery=25000;
  if(scenario==0)result.crew[0].condition.internal[9]=0;
  if(scenario==3){for(auto &member:result.crew){member.condition.armour.fill(40);member.condition.internal.fill(40);member.condition.internal[9]=100;}}
  shell.mission_return(result,mission->mission_variables());
  require(shell.presentation_active(),"campaign outcome starts its presentation");
  if(scenario<3){
   require(shell.screen()==0,"terminal outcome starts cinematic");frame(shell,argv[2],"ending-"+std::to_string(scenario)+".ppm");skip(shell);
   require(shell.screen()==1200,"terminal cinematic returns to careers");
   if(scenario==1)require(shell.state().returned().continuation==99,"success selects victory");
   if(scenario==2)require(shell.state().returned().continuation==-1,"failure selects defeat");
  }else{
   frame(shell,argv[2],"mechanic-return-"+std::to_string(scenario)+".ppm");skip(shell);
   require(shell.screen()==1520,"mechanic responses precede debrief");
   if(scenario==3)require(shell.state().returned().condition<51,"mechanic uses pre-repair condition");
   if(scenario==4){
    shell.action(1710);shell.action(1499);require(shell.presentation_active(),"next chapter starts mechanic introduction");
    shell.key(1);require(shell.screen()==1490,"chapter introduction leads to general");shell.key(1);
    require(shell.screen()==0,"general leads to travel movie");frame(shell,argv[2],"chapter-travel.ppm");
    skip(shell);require(shell.screen()==1700,"travel leads to service bay");
   }
  }
 }
 std::cout<<"Full startup, skip boundaries, campaign return, death, victory, defeat and chapter travel pass\n";return 0;
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return 1;}
