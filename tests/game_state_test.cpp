#include "shell/game_state.h"
#include "resources/career_index.h"
#include <algorithm>
#include <iostream>
#include <stdexcept>
using namespace earthsiege;
namespace {
void require(bool ok,std::string const &what){if(!ok)throw std::runtime_error{what};}
bytes blob(binary_reader &f){return f.take(f.integer<uint32_t>());}
void compare(bytes actual,bytes expected,std::string const &what){
 if(std::ranges::equal(actual,expected))return;
 auto diff{std::ranges::mismatch(actual,expected)};
 std::cerr<<what<<" sizes "<<actual.size()<<'/'<<expected.size()<<" difference "<<diff.in1-actual.begin()<<'\n';
 throw std::runtime_error{"original shell state mismatch"};
}
random_state::state random(binary_reader &f){random_state::state s;for(auto &w:s.words)w=f.integer<uint16_t>();s.first=f.integer<uint8_t>();s.second=f.integer<uint8_t>();return s;}
void compare(random_state::state const &a,random_state::state const &b){require(a.words==b.words&&a.first==b.first&&a.second==b.second,"original shell RNG mismatch");}
}
int main(int argc,char **argv)try{
 if(argc!=3)return 1;
 resource_store resources{argv[1]};auto data{read_file(argv[2])};binary_reader f{data};auto m{f.take(8)};require(std::string(m.begin(),m.end())=="ESSHL001","fixture version");
 auto count{f.integer<uint32_t>()};
 for(unsigned i{};i<count;++i){
  auto seed{f.integer<int16_t>()},mode{f.integer<int16_t>()},category{f.integer<int16_t>()},battle{f.integer<int16_t>()};auto name{f.cstring()};
  auto script{blob(f)},counters{blob(f)},player{blob(f)},crew{blob(f)},final_player{blob(f)};auto before{random(f)},after{random(f)};
  game_state game{resources,seed};if(mode)game.new_campaign(battle);else game.prepare_single(category,{});
  require(normalise_path("msn/"+game.mission_name()+".msn")==normalise_path(name),"original random mission choice");
  compare(write_mission_script(game.prepared().compiled.script),script,name+" script");compare(write_mission_counters(game.prepared().counters),counters,name+" counters");
  compare(write_player_roster(game.player()),player,name+" prepared player");
  if(mode)compare(write_career_state(game.crew()),crew,name+" selected crew");
  compare(game.random_snapshot(),before);game.update_roster();compare(write_player_roster(game.player()),final_player,name+" final player");compare(game.random_snapshot(),after);
 }
 f.require_end();
 auto const original_index{resources.read("shell0","car/names.log")};
 compare(write_career_index(read_career_index(original_index)),original_index,"supplied career index");
 std::cout<<count<<" original complete shell choices, crew activation and final roster exports match\n";
 return 0;
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return 1;}
