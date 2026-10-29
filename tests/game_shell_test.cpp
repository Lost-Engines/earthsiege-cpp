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
 if(argc!=3&&argc!=4)return 1;
 resource_store resources{argv[1]};temporary_saves root{argv[2]};game_shell shell{resources,root.path,17};
 screen(shell,1000);require(!shell.current_dialog().find(1700)->enabled,"no career to resume");
 click(shell,1100);screen(shell,1100);shell.key(80);require(shell.current_dialog().selection(1101)==1,"mission keyboard selection");
 click(shell,1110);screen(shell,1110);auto original{shell.settings().current()};click(shell,1111);require(shell.settings().current()[12]!=original[12],"single option changes");shell.key(1);screen(shell,1100);require(shell.settings().current()==original,"single cancel rolls back");
 click(shell,1501);screen(shell,1501);require(shell.state().ready()&&!shell.state().campaign(),"single mission prepared");click(shell,1800);screen(shell,1800);
 auto idle_random{shell.state().random_snapshot()};
 for(unsigned i{};i<240;++i)shell.advance(3);
 require(idle_random.words!=shell.state().random_snapshot().words||idle_random.first!=shell.state().random_snapshot().first,"resident bay scripts advance in live shell");
 auto initial_slot{shell.state().roster().selected};auto const initial_herc{*shell.state().roster().find(initial_slot)};
 auto const old_weapon{initial_herc.weapons[0]};auto const old_count{shell.state().inventory()[static_cast<size_t>(old_weapon)].size()};
 armament_catalog catalog{resources};auto const &region{catalog.hotspots(initial_herc.chassis)[0]};
 int const mx{(region.left+region.right)/2},my{(region.top+region.bottom)/2};shell.mouse_down(mx,my);shell.mouse_up(mx,my);
 for(int i{};i<18;++i)shell.key(72);
 require(shell.state().roster().find(initial_slot)->weapons[0]==17,"remove weapon through actual input");
 require(shell.state().inventory()[static_cast<size_t>(old_weapon)].size()==old_count+1,"removed weapon returned to inventory");
 shell.key(80);require(shell.state().roster().find(initial_slot)->weapons[0]!=17,"equip weapon through actual input");
 if(argc==4){framebuffer frame;shell.draw(frame);frame.save(argv[3],shell.colours());}
 click(shell,2004);require(shell.state().roster().selected!=initial_slot,"single mission HERC changes");
 require(shell.state().crew().crew[0].hangar_slot==shell.state().roster().selected,"single mission selected HERC assigned");
 click(shell,100);screen(shell,1501);
 click(shell,100);require(shell.ready_to_launch(),"single launch request");auto single{shell.launch()};require(single!=nullptr&&!shell.ready_to_launch(),"launch consumed");shell.mission_return(single->results(),single->mission_variables());screen(shell,1510);
 click(shell,1000);click(shell,1200);screen(shell,1200);click(shell,1210);screen(shell,1210);shell.text_input("Test Pilot");click(shell,100);screen(shell,1700);require(shell.state().campaign(),"registered campaign");auto name{shell.state().mission_name()};
 framebuffer bay_before;shell.draw(bay_before);bool moving{};
 for(unsigned i{};i<600&&!moving;++i){shell.advance(3);framebuffer f;shell.draw(f);moving=f.pixels()!=bay_before.pixels();}
 require(moving,"original workers animate in the service bay");
 auto crew_before{shell.state().crew()};
 click(shell,1610);screen(shell,1610);
 if(argc==4){framebuffer frame;shell.draw(frame);auto path{std::filesystem::path{argv[3]}};path.replace_filename("crew-test.ppm");frame.save(path,shell.colours());}
 click(shell,2004);
 require(shell.state().crew().crew[0].hangar_slot==crew_before.crew[1].hangar_slot,"crew swaps selected HERC");
 require(shell.state().crew().crew[1].hangar_slot==crew_before.crew[0].hangar_slot,"displaced pilot gets previous HERC");
 click(shell,2004);
 require(shell.state().crew().crew[1].hangar_slot==crew_before.crew[1].hangar_slot,"cycling restores previous displaced pilot");
 click(shell,1700);screen(shell,1700);
 for(size_t i{};i<4;++i)require(shell.state().crew().crew[i].hangar_slot==crew_before.crew[i].hangar_slot,"crew cancel restores assignments");
 click(shell,1610);click(shell,1616);click(shell,1623);
 require(shell.state().crew().crew[1].hangar_slot==-1,"wingmate can be unassigned");
 click(shell,1700);screen(shell,1700);
 click(shell,1610);click(shell,2004);click(shell,100);screen(shell,1700);
 require(shell.state().crew().crew[0].hangar_slot==crew_before.crew[1].hangar_slot,"crew accept retains swap");

 click(shell,100);auto mission{shell.launch()};auto result{mission->results()};auto counters{mission->mission_variables()};result.grade=1;result.recovery+=60000;for(auto &c:result.crew){c.condition.armour.fill(60);c.condition.internal.fill(70);c.condition.internal[9]=100;c.condition.equipment.fill(55);}shell.mission_return(result,counters,13);screen(shell,1520);require(shell.state().crew().missions==1,"return advances career");

 click(shell,1710);screen(shell,1710);click(shell,1900);screen(shell,1900);
 auto repair_slot{shell.state().roster().selected};auto repair_before{*shell.state().roster().find(repair_slot)};auto repair_budget{shell.state().crew().budget};
 click(shell,1920);screen(shell,1920);click(shell,106);
 require(shell.state().crew().budget<repair_budget,"component repair spends material");
 if(argc==4){framebuffer frame;shell.draw(frame);auto path{std::filesystem::path{argv[3]}};path.replace_filename("component-repair-test.ppm");frame.save(path,shell.colours());}
 click(shell,99);screen(shell,1900);
 require(shell.state().crew().budget==repair_budget&&shell.state().roster().find(repair_slot)->condition.internal==repair_before.condition.internal,"component cancel restores material and conditions");
 click(shell,1920);click(shell,106);click(shell,1900);screen(shell,1900);
 require(shell.state().roster().find(repair_slot)->condition.internal[0]==91,"accept retains native extra repair band");
 if(argc==4){framebuffer frame;shell.draw(frame);auto path{std::filesystem::path{argv[3]}};path.replace_filename("repair-test.ppm");frame.save(path,shell.colours());}
 click(shell,1910);screen(shell,1910);click(shell,1911);screen(shell,1900);require(shell.state().crew().repair_policy==0,"manual repair policy");
 auto repair_count{shell.state().roster().hercs.size()};
 click(shell,1901);screen(shell,4444);click(shell,1900);screen(shell,1900);require(shell.state().roster().hercs.size()==repair_count,"scrap cancellation leaves roster intact");
 click(shell,1902);require(shell.state().roster().find(repair_slot)->condition.internal[0]==100,"whole HERC repair");
 click(shell,1710);screen(shell,1710);click(shell,2000);screen(shell,2000);
 auto build_budget{shell.state().crew().budget};auto build_count{shell.state().roster().hercs.size()};
 require(shell.state().roster().construction_slot>=0,"construction reserves an empty bay");
 click(shell,2001);screen(shell,2000);
 require(shell.state().roster().hercs.size()==build_count+1&&shell.state().crew().budget==build_budget-50000,"construction spends material");
 require(!shell.current_dialog().find(2004)->enabled,"cannot cycle during construction");
 if(argc==4){framebuffer frame;shell.draw(frame);auto path{std::filesystem::path{argv[3]}};path.replace_filename("construction-test.ppm");frame.save(path,shell.colours());}
 click(shell,2001);
 require(shell.state().roster().hercs.size()==build_count&&shell.state().crew().budget==build_budget,"immediate cancellation returns construction allocation");
 click(shell,2001);click(shell,1710);screen(shell,1710);click(shell,1000);screen(shell,1000);

 game_shell reopened{resources,root.path,23};require(reopened.current_dialog().find(1700)->enabled,"resume survives restart");click(reopened,1700);screen(reopened,1710);require(reopened.state().crew().missions==1,"original save loaded");require(reopened.state().roster().construction_remaining==1,"construction survives reload");click(reopened,1499);screen(reopened,1700);require(reopened.state().crew().step==1,"next mission advances step");require(reopened.state().mission_name()!=name,"next mission differs");
 click(reopened,1000);click(reopened,1200);click(reopened,1220);screen(reopened,1220);click(reopened,1221);require(reopened.state().career().career.states[2].valid,"manual backup stored");
 click(reopened,1222);require(reopened.state().crew().step==1,"manual backup restored");
 // Resume genuine save-format careers into each original validation path.
 auto seed{shell.state().career()};
 auto const entry{save_store{root.path}.index().entries.front()};
 for(int scenario{};scenario<5;++scenario){
  auto saved{seed};auto &c{saved.career.states[0]};auto &h{saved.hangars[0]};
  h=read_hangar(resources.read("shell0","car/sm_init.dat"));h.selected=0;c.selected_crew=0;c.crew_cursor=0;c.screen=scenario<2?1700:1710;
  for(size_t i{};i<4;++i){c.crew[i].active=1;c.crew[i].hangar_slot=static_cast<int16_t>(i);}
  if(scenario==0)c.crew[0].hangar_slot=-1;
  if(scenario==1)std::ranges::fill(h.find(0)->weapons,int16_t{17});
  if(scenario==2)h.find(0)->condition.internal[0]=49;
  if(scenario==3)h.find(1)->condition.internal[5]=49;
  save_store{root.path}.save(entry,saved);
  game_shell guarded{resources,root.path,17};click(guarded,1700);screen(guarded,c.screen);
  if(scenario==0){
   click(guarded,100);screen(guarded,3333);require(!guarded.ready_to_launch(),"missing player blocks launch");click(guarded,99);screen(guarded,1700);
  }else if(scenario==1){
   click(guarded,100);screen(guarded,3000);guarded.key(1);screen(guarded,1700);require(!guarded.ready_to_launch(),"unarmed warning cancel");
   click(guarded,1500);screen(guarded,1500);click(guarded,100);screen(guarded,3000);
   if(argc==4){framebuffer frame;guarded.draw(frame);auto path{std::filesystem::path{argv[3]}};path.replace_filename("launch-warning-test.ppm");frame.save(path,guarded.colours());}
   click(guarded,100);require(guarded.ready_to_launch(),"unarmed warning permits proceed");
  }else if(scenario==2){
   click(guarded,1499);screen(guarded,7770);click(guarded,99);screen(guarded,1710);require(guarded.state().crew().step==c.step,"disabled player leaves mission unchanged");
  }else if(scenario==3){
   click(guarded,1499);screen(guarded,7771);guarded.key(1);screen(guarded,1710);require(guarded.state().crew().step==c.step,"disabled wingmate cancel");
   click(guarded,1499);click(guarded,1499);screen(guarded,1700);require(guarded.state().crew().step!=c.step,"disabled wingmate proceed");
  }else{
   click(guarded,2000);screen(guarded,7772);click(guarded,99);screen(guarded,1710);require(guarded.state().roster().construction_slot==-1,"full hangar cannot reserve construction");
  }
 }
 std::cout<<"Single mission and career launch/return, isolated persistence, preferences and backups pass\n";return 0;
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return 1;}
