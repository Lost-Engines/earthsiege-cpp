#include "shell/mission_launch.h"
#include "resources/campaign.h"
#include <algorithm>
#include <iostream>
#include <stdexcept>
using namespace earthsiege;
namespace {
bytes block(binary_reader &r){return r.take(r.integer<uint32_t>());}
void equal(bytes a,bytes b,std::string const &label){if(std::ranges::equal(a,b))return;auto m{std::ranges::mismatch(a,b)};throw std::runtime_error{label+" sizes "+std::to_string(a.size())+"/"+std::to_string(b.size())+" difference "+std::to_string(m.in1-a.begin())+" values "+(m.in1==a.end()?"EOF":std::to_string(*m.in1))+"/"+(m.in2==b.end()?"EOF":std::to_string(*m.in2))};}
}
int main(int argc,char **argv)try{
 if(argc!=3)return 1;
 resource_store resources{argv[1]};auto data{read_file(argv[2])};binary_reader f{data};f.take(8);auto n{f.integer<uint16_t>()};
 for(unsigned i{};i<n;++i){auto name{f.cstring()};auto mode{f.integer<int16_t>()},node{f.integer<int16_t>()},step{f.integer<int16_t>()},seed{f.integer<int16_t>()};auto script{block(f)},player{block(f)},variables{block(f)};
  random_state random{seed};std::optional<campaign_start> career;if(mode)career=create_career(resources,1,0,random,1);
  auto mission{prepare_mission(resources.read("zones","msn/"+name+".msn"),mode,node,step,{}, {},random,game_edition::expansion,1,1)};
  equal(write_mission_counters(mission.counters),variables,name+" counters");equal(write_mission_script(mission.compiled.script),script,name+" script");
  player_roster actual;
  if(mode)actual=launch_player_roster(career->career.states[0],career->hangars[0],true,0,random);
  else {auto crew{generate_single_crew(resources,mission.compiled.script,random,1)};actual=launch_player_roster(crew.state,crew.roster,false,0,random);}
  equal(write_player_roster(actual),player,name+" player");
 }
 f.require_end();auto campaign{read_campaign_table(resources.read("zones","bst/btlset_1.dat"),true)};
 for(int16_t node{};node<3;++node)for(int16_t step{};step<8;++step)for(bool fail:{false,true}){auto next{node},frame{step};auto changed{campaign.advance(next,frame,fail)};auto expected{step<7?node:fail?-1:node==2?99:node+1};if(next!=expected||frame!=(step+1)%8||changed!=(step==7))throw std::runtime_error{"expansion chapter transition"};}
 std::cout<<n<<" original expansion launches match script, variables and crew; all campaign transitions checked\n";return 0;
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return 1;}
