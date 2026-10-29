#include "shell/mission_launch.h"
#include "resources/binary_writer.h"
#include <algorithm>
#include <iostream>
#include <stdexcept>
using namespace earthsiege;
namespace {
void require(bool ok,std::string const &message){if(!ok)throw std::runtime_error{message};}
bytes blob(binary_reader &f){auto const n{f.integer<uint32_t>()};return f.take(n);}
random_state::state rng(binary_reader &f){random_state::state s;for(auto &w:s.words)w=f.integer<uint16_t>();s.first=f.integer<uint8_t>();s.second=f.integer<uint8_t>();return s;}
void random_matches(random_state const &a,random_state::state const &b,std::string const &name){auto const &s{a.snapshot()};require(s.words==b.words&&s.first==b.first&&s.second==b.second,"native RNG mismatch: "+name);}
void compare(bytes actual,bytes expected,std::string const &name){if(std::ranges::equal(actual,expected))return;auto diff{std::ranges::mismatch(actual,expected)};
  std::cerr<<name<<" sizes "<<actual.size()<<'/'<<expected.size()<<", first difference "<<diff.in1-actual.begin();if(diff.in1!=actual.end()&&diff.in2!=expected.end())std::cerr<<": "<<int(*diff.in1)<<'/'<<int(*diff.in2);std::cerr<<'\n';throw std::runtime_error{"native launch byte mismatch"};}
void magic(binary_reader &f,std::string const &expected){auto const m{f.take(8)};require(std::string(m.begin(),m.end())==expected,"fixture version");}
}
int main(int argc,char **argv)try {
  if(argc!=5)return 1;
  resource_store resources{argv[1]};auto const new_data{read_file(argv[2])};binary_reader nf{new_data};magic(nf,"ESNEW001");
  auto const constructors{nf.integer<uint32_t>()};
  for(unsigned i{};i<constructors;++i){auto const seed{nf.integer<int16_t>()},battle{nf.integer<int16_t>()},policy{nf.integer<int16_t>()};auto const runtime{blob(nf)},saved{blob(nf)},hangar_file{blob(nf)};auto const expected_rng{rng(nf)};
    random_state random{seed};auto const created{create_career(resources,battle,policy,random)};
    compare(write_career(created.career),runtime,"new career runtime "+std::to_string(seed));compare(created.initial_career_file(),saved,"new career file");
    binary_writer h;for(auto const &roster:created.hangars)h.append(write_hangar(roster));compare(std::move(h).finish(),hangar_file,"new hangar history");random_matches(random,expected_rng,"new career");
  }
  nf.require_end();auto const launch_data{read_file(argv[3])};binary_reader file{launch_data};magic(file,"ESLCH001");auto const launches{file.integer<uint32_t>()};
  for(unsigned i{};i<launches;++i){auto const name{file.cstring()};auto const mode{file.integer<int16_t>()},node{file.integer<int16_t>()},step{file.integer<int16_t>()},seed{file.integer<int16_t>()};
    mission_options const options{file.integer<int16_t>(),file.integer<int16_t>(),file.integer<int16_t>(),file.integer<int16_t>(),file.integer<int16_t>()};mission_counters history{};for(auto &v:history)v=file.integer<int16_t>();
    auto const before{rng(file)},after{rng(file)};auto const expected_script{blob(file)},expected_counters{blob(file)},expected_player{blob(file)};
    random_state random{seed};std::optional<campaign_start> career;if(mode)career=create_career(resources,0,0,random);
    random_matches(random,before,name+" before compilation");auto const prepared{prepare_mission(resources.read("zones","msn/"+name+".msn"),mode,node,step,options,history,random)};
    compare(write_mission_script(prepared.compiled.script),expected_script,name+" script");compare(write_mission_counters(prepared.counters),expected_counters,name+" counters");
    player_roster player;
    if(mode)player=launch_player_roster(career->career.states[0],career->hangars[0],true,0,random);
    else{auto crew{generate_single_crew(resources,prepared.compiled.script,random)};player=launch_player_roster(crew.state,crew.roster,false,0,random);}
    compare(write_player_roster(player),expected_player,name+" player");random_matches(random,after,name+" after launch");
  }
  file.require_end();
  auto const roster_data{read_file(argv[4])};binary_reader rf{roster_data};magic(rf,"ESLRS001");auto const roster_cases{rf.integer<uint32_t>()};
  for(unsigned i{};i<roster_cases;++i){auto const mode{rf.integer<int16_t>()},identity{rf.integer<int16_t>()};auto const state_data{blob(rf)},roster_bytes{blob(rf)},player{blob(rf)},after{blob(rf)};
    random_state random{rng(rf)};auto const expected_rng{rng(rf)};binary_reader state_reader{state_data};auto const state{read_career_state(state_reader)};state_reader.require_end();auto roster{read_hangar(roster_bytes)};
    auto const actual{launch_player_roster(state,roster,mode!=0,identity,random)};compare(write_player_roster(actual),player,"altered roster "+std::to_string(i));compare(write_hangar(roster),after,"launch hangar mutation "+std::to_string(i));random_matches(random,expected_rng,"altered roster");
  }
  rf.require_end();std::cout<<constructors<<" complete original career constructors and "<<launches<<" coherent original mission launches and "<<roster_cases<<" altered rosters match byte for byte, with exact RNG state\n";return 0;
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return 1;}
