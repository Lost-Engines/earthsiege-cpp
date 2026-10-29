#include "shell/campaign_return.h"
#include "resources/resource_store.h"
#include "resources/binary_writer.h"
#include <algorithm>
#include <iostream>
#include <stdexcept>
using namespace earthsiege;
static void require(bool ok,char const *message){if(!ok)throw std::runtime_error{message};}
static bytes blob(binary_reader &f){auto const n{f.integer<uint32_t>()};return f.take(n);}
static void compare(bytes actual,bytes expected,unsigned case_id,char const *name) {
  if(std::ranges::equal(actual,expected))return;
  auto diff{std::ranges::mismatch(actual,expected)};auto const offset{diff.in1-actual.begin()};
  std::cerr<<"case "<<case_id<<' '<<name<<" sizes "<<actual.size()<<'/'<<expected.size()<<" differ at "<<offset;
  if(diff.in1!=actual.end()&&diff.in2!=expected.end())std::cerr<<": "<<int(*diff.in1)<<" != "<<int(*diff.in2);
  std::cerr<<'\n';throw std::runtime_error{"native return transaction mismatch"};
}
static random_state::state rng(binary_reader &f){random_state::state s;for(auto &w:s.words)w=f.integer<uint16_t>();s.first=f.integer<uint8_t>();s.second=f.integer<uint8_t>();return s;}
int main(int argc,char **argv)try {
  if(argc!=3)return 1;
  resource_store resources{argv[1]};auto const data{read_file(argv[2])};binary_reader file{data};auto const magic{file.take(8)};
  require(std::string(magic.begin(),magic.end())=="ESRET001","fixture version");auto const count{file.integer<uint32_t>()};
  auto const prices{read_service_prices(resources.read("shell0","cfg/damage.dat"))};
  for(unsigned i{};i<count;++i){
    auto const initial_career{blob(file)},initial_hangar{blob(file)},result_data{blob(file)},saved{blob(file)},hangar_after{blob(file)};
    std::array<int16_t,100> counters{};for(auto &v:counters)v=file.integer<int16_t>();random_state random{rng(file)};auto const expected_rng{rng(file)};
    auto const deaths{file.integer<int16_t>()};std::array<int16_t,3> totals{};for(auto &v:totals)v=file.integer<int16_t>();
    auto career{read_career(initial_career)};binary_reader hangars{initial_hangar};std::array<hangar,41> history;
    for(auto &h:history)h=read_hangar(hangars);
    hangars.require_end();auto const results{read_mission_results(result_data)};compare(write_mission_results(results),result_data,i,"result serializer");
    auto const debrief{return_from_mission(career,history[0],results,counters,prices,random)};
    compare(write_career(career),saved,i,"career");binary_writer encoded;for(auto const &h:history)encoded.append(write_hangar(h));compare(std::move(encoded).finish(),hangar_after,i,"hangar");
    require(debrief.deaths==deaths&&debrief.statistics==totals,"native debrief statistics mismatch");
    auto const &state{random.snapshot()};require(state.words==expected_rng.words&&state.first==expected_rng.first&&state.second==expected_rng.second,"native replacement RNG mismatch");
  }
  file.require_end();std::cout<<count<<" complete original mission return/save transactions match, including budgets, crew, all inventories and RNG state\n";return 0;
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return 1;}
