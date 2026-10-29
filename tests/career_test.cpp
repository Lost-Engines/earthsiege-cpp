#include "resources/career.h"
#include "resources/campaign.h"
#include "resources/resource_store.h"
#include <algorithm>
#include <iostream>
#include <stdexcept>
using namespace earthsiege;
static void require(bool ok,char const *message){if(!ok)throw std::runtime_error{message};}
static bytes blob(binary_reader &file){auto const n{file.integer<uint32_t>()};return file.take(n);}
static bool equal(bytes a,bytes b){return std::ranges::equal(a,b);}
int main(int argc,char **argv)try {
  if(argc!=4)return 1;
  resource_store resources{argv[1]};auto const fixture{read_file(argv[2])};binary_reader file{fixture};
  auto const magic{file.take(8)};require(std::string(magic.begin(),magic.end())=="ESCAR001","career fixture version");
  auto const count{file.integer<uint32_t>()};
  for(unsigned i{};i<count;++i) {
    auto const history{file.integer<uint16_t>()};auto const input{blob(file)},saved{blob(file)},restored{blob(file)};
    auto career{read_career(input)};std::array<int16_t,100> counters{};for(int16_t j{};j<100;++j)counters[j]=j;
    checkpoint_career(career,history,11,123456,counters,2);
    require(equal(write_career(career),saved),"native complete career writer mismatch");
    require(equal(write_career(read_career(saved)),restored),"native complete career reader mismatch");
    // A malformed save must not become an apparently valid current career.
    try {read_career(saved.first(saved.size()-1));throw std::logic_error{"truncated career accepted"};}
    catch(std::runtime_error const &){}
  }
  file.require_end();
  for(auto const name:{"car/car_init.dat","car/sm_init.dat"}) {
    auto const original{resources.read("shell0",name)};
    require(equal(write_hangar(read_hangar(original)),original),"original starting hangar serialization mismatch");
  }
  auto const campaign_fixture{read_file(argv[3])};binary_reader branch_file{campaign_fixture};auto const m{branch_file.take(8)};
  require(std::string(m.begin(),m.end())=="ESCAM001","campaign fixture version");auto const transitions{branch_file.integer<uint32_t>()};
  for(unsigned i{};i<transitions;++i) {
    auto const table_name{branch_file.cstring()},expected_name{branch_file.cstring()};
    auto node{branch_file.integer<int16_t>()},step{branch_file.integer<int16_t>()};auto const branch{branch_file.integer<int16_t>()};
    auto const changed{branch_file.integer<int16_t>()},newstep{branch_file.integer<int16_t>()},newnode{branch_file.integer<int16_t>()};
    auto const table{read_campaign_table(resources.read("zones","bst/"+table_name))};
    require(table.filename(node,step,branch)!=std::string{},"missing campaign mission");
    require(table.filename(node,step,branch)==expected_name,"native campaign choice mismatch");
    require(table.advance(node,step,branch)==bool(changed)&&node==newnode&&step==newstep,"native campaign advance mismatch");
  }
  auto const kinds{branch_file.integer<uint32_t>()};
  for(unsigned kind{};kind<kinds;++kind){auto const threshold{branch_file.integer<int16_t>()};for(int16_t grade{-2};grade<12;++grade)require(campaign_failure_branch(static_cast<int16_t>(kind),grade)==(grade>threshold),"native campaign threshold mismatch");}
  branch_file.require_end();std::cout<<count<<" original complete career saves/loads, both starting hangars, "<<transitions<<" original campaign transitions and "<<kinds*14<<" thresholds match\n";
  return 0;
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return 1;}
