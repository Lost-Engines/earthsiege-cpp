#include "shell/game_shell.h"
#include <chrono>
#include <iostream>
#include <stdexcept>
using namespace earthsiege;
namespace {
void check(bool pass,char const *message){if(!pass)throw std::runtime_error{message};}
void skip(game_shell &s){for(int i{};s.presentation_active()&&i<100;++i){if(s.transition_active())s.advance(20);else s.key(1);}check(!s.presentation_active(),"presentation skip");s.take_audio();}
void action(game_shell &s,uint16_t id){s.action(id);skip(s);framebuffer f;s.draw(f);}
}
int main(int argc,char **argv)try{
 if(argc!=3)return 1;
 resource_store r{argv[1]};auto root{std::filesystem::path{argv[2]}/("expansion-shell-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()))};
 struct cleanup{std::filesystem::path p;~cleanup(){std::error_code e;std::filesystem::remove_all(p,e);}} guard{root};
 auto roster{read_hangar(r.read("shell0","bar/sm_init.dat"),r.edition())};armament_catalog mounts{r};hangar_artwork art{r};
 for(auto h:roster.hercs){framebuffer f;art.draw(f,h,mounts);h.condition.armour.fill(30);art.damage(f,h);}
 auto apocalypse{roster.find(8)};check(apocalypse&&apocalypse->chassis==16,"Apocalypse shell slot");check(mounts.mounts(16).size()==9,"Apocalypse mounts");
 auto tech{read_technology(r.read("zones","bst/tech_1.dat"),r.edition(),1)};for(auto &v:tech.entries)v.available=1;armory stock{22};
 for(size_t i{};i<stock.size();++i)stock[i].push_back(100);
 for(int16_t weapon{18};weapon<22;++weapon){bool fitted{};for(int i{};i<9;++i)if(mounts.find(16,i,weapon)){check(mounts.exchange(*apocalypse,i,weapon,stock),"upgraded weapon fitting");framebuffer f;art.draw(f,*apocalypse,mounts,i);fitted=true;break;}check(fitted,"Apocalypse upgraded weapon compatibility");}
 game_shell shell{r,root,17};shell.startup(true);skip(shell);action(shell,1100);action(shell,1501);check(shell.state().mission_name().starts_with("s2_"),"expansion single mission selection");action(shell,1800);
 for(int i{};i<12;++i)action(shell,2004);
 action(shell,100);action(shell,100);check(shell.ready_to_launch(),"single mission launch");auto mission{shell.launch()};mission->step({},256);auto result{mission->results()};shell.mission_return(result,mission->mission_variables());skip(shell);
 action(shell,1000);action(shell,1200);action(shell,1210);shell.text_input("Expansion Pilot");action(shell,100);check(shell.state().mission_name()=="B1_C0_0A","expansion career first mission");check(shell.state().crew().budget==100000,"expansion campaign budget");action(shell,1800);action(shell,100);action(shell,100);check(shell.ready_to_launch(),"career launch");mission=shell.launch();result=mission->results();result.grade=0;shell.mission_return(result,mission->mission_variables());skip(shell);check(shell.state().crew().missions==1,"expansion career return");action(shell,1710);action(shell,1000);
 game_shell reopened{r,root,23};action(reopened,1700);check(reopened.state().crew().missions==1,"expansion save and resume");action(reopened,1499);check(reopened.state().crew().step==1,"expansion next mission");

 save_store source_store{root,game_edition::expansion};random_state transfer_rng{42};auto veteran{create_career(r,0,1,transfer_rng,2)};veteran.career.states[0].crew[0].total[0]=50;veteran.career.states[0].budget=345678;
 career_index source_index;source_index.selected=0;source_index.entries.push_back({"Veteran",0,"car/car_0.car","car/car_0.mec"});source_store.save(source_index.entries[0],veteran);source_store.save_index(source_index);
 action(reopened,1000);action(reopened,1300);action(reopened,1330);action(reopened,1340);action(reopened,1350);check(reopened.screen()==1350,"transfer equipment choice");action(reopened,1351);check(reopened.state().crew().budget==345678&&reopened.state().crew().crew[0].total[0]==50,"transfer retains salvage and pilot history");
 action(reopened,1000);action(reopened,1300);action(reopened,1330);action(reopened,1340);action(reopened,1350);check(reopened.screen()==1360,"transfer overwrite warning");action(reopened,1350);action(reopened,1352);check(reopened.state().crew().budget==100000&&reopened.state().crew().crew[0].total[0]==50,"fresh squad retains pilot history");
 std::cout<<"All nine service-bay chassis, four upgraded weapons, single/career launch and return, save/resume pass\n";return 0;
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return 1;}
