#include "shell/campaign_return.h"
#include <iostream>
using namespace earthsiege;
namespace {
bytes blob(binary_reader &f){return f.take(f.integer<uint32_t>());}
void compare(bytes a,bytes b,std::string const &name){
 if(!std::ranges::equal(a,b)){auto m{std::ranges::mismatch(a,b)};throw std::runtime_error{name+" byte "+std::to_string(m.in1-a.begin())};}
}
}
int main(int argc,char **argv)try{
 if(argc!=2)return 1;
 auto raw{read_file(argv[1])};binary_reader f{raw};auto m{f.take(8)};if(std::string(m.begin(),m.end())!="ESSRET01")f.fail("fixture");
 auto count{f.integer<uint32_t>()};
 for(unsigned i{};i<count;++i){
  auto state_raw{blob(f)};binary_reader reader{state_raw};auto state{read_career_state(reader)};reader.require_end();
  auto roster{read_hangar(blob(f))};auto results{read_mission_results(blob(f))};auto counters{blob(f)};
  auto expected_state{blob(f)},expected_roster{blob(f)},expected_counters{blob(f)};
  auto deaths{f.integer<int16_t>()};std::array<int16_t,3> totals;for(auto &v:totals)v=f.integer<int16_t>();
  auto result{return_from_single_mission(state,roster,results)};
  if(result.deaths!=deaths||result.statistics!=totals)f.fail("original single-mission debrief");
  compare(write_career_state(state),expected_state,"single state "+std::to_string(i));
  compare(write_hangar(roster),expected_roster,"single roster "+std::to_string(i));
  std::vector<uint8_t> actual_counters{counters.begin(),counters.end()};actual_counters[0]=static_cast<uint8_t>(results.grade);actual_counters[1]=static_cast<uint8_t>(results.grade>>8);
  compare(actual_counters,expected_counters,"single result counters");
 }
 f.require_end();std::cout<<count<<" original single-mission return transactions agree\n";return 0;
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return 1;}
