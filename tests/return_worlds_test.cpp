#include "simulation/mission_session.h"
#include "resources/input_tape.h"
#include "resources/resource_store.h"
#include <algorithm>
#include <iostream>
#include <stdexcept>
using namespace earthsiege;
int main(int argc,char **argv)try{
 if(argc!=3)return 1;
 resource_store resources{argv[1]};auto const data{read_file(argv[2])};binary_reader f{data};auto const magic{f.take(8)};if(std::string(magic.begin(),magic.end())!="ESRETN01")f.fail("fixture version");auto const count{f.integer<uint32_t>()};
 for(unsigned i{};i<count;++i){auto const demo{f.cstring()};random_state::state rng;for(auto &v:rng.words)v=f.integer<uint16_t>();rng.first=f.integer<uint8_t>();rng.second=f.integer<uint8_t>();mission_counters counters;for(auto &v:counters)v=f.integer<int16_t>();auto const expected{f.take(f.integer<uint32_t>())};auto const tape{read_input_tape(read_file(resources.root()/"TAPES"/(demo+".TAP")))};auto const script{read_mission_script(tape.starting_files[std::to_underlying(tape_file::script)])};auto const roster{read_player_roster(tape.starting_files[std::to_underlying(tape_file::player)])};auto const input_counters{read_mission_counters(tape.starting_files[std::to_underlying(tape_file::campaign_flags)])};mission_session session{resources,script,roster,random_state{rng},input_counters};
  auto const result{write_mission_results(session.results())};if(!std::ranges::equal(result,expected)){auto const d{std::ranges::mismatch(result,expected)};std::cerr<<demo<<" difference at "<<d.in1-result.begin()<<" sizes "<<result.size()<<'/'<<expected.size()<<'\n';f.fail("native world results");}if(session.mission_variables()!=counters)f.fail("native world counters");
  if(write_mission_results(session.results())!=result)f.fail("repeated result query changed salvage");
 }
 f.require_end();std::cout<<count<<" complete original pre-tick mission handoffs agree\n";return 0;
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return 1;}
