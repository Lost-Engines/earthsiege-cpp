#include "shell/new_career.h"
#include "resources/binary_writer.h"
#include <algorithm>
#include <iostream>
#include <stdexcept>
using namespace earthsiege;
namespace {
bytes blob(binary_reader &r){return r.take(r.integer<uint32_t>());}
void require(bool ok,std::string const &why){if(!ok)throw std::runtime_error{why};}
void compare(bytes a,bytes b,std::string const &why){
 if(std::ranges::equal(a,b))return;
 auto m{std::ranges::mismatch(a,b)};auto offset{m.in1-a.begin()};
 throw std::runtime_error{why+" sizes "+std::to_string(a.size())+"/"+std::to_string(b.size())+" first difference "+std::to_string(offset)+" values "+(m.in1==a.end()?"EOF":std::to_string(*m.in1))+"/"+(m.in2==b.end()?"EOF":std::to_string(*m.in2))};
}
}
int main(int argc,char **argv)try{
 if(argc!=3)return 1;
 resource_store resources{argv[1]};auto data{read_file(argv[2])};binary_reader r{data};
 auto m{r.take(8)};require(std::string(m.begin(),m.end())=="ESEXCAR1","fixture magic");auto count{r.integer<uint16_t>()};
 for(unsigned i{};i<count;++i){
  auto seed{r.integer<int16_t>()},battle{r.integer<int16_t>()},difficulty{r.integer<int16_t>()},policy{r.integer<int16_t>()};
  auto saved{blob(r)},hangars{blob(r)},current{blob(r)};
  random_state random{seed};auto created{create_career(resources,battle,policy,random,static_cast<uint8_t>(difficulty))};
  compare(created.initial_career_file(),saved,"original initial career "+std::to_string(i));
  binary_writer h;for(auto const &roster:created.hangars)h.append(write_hangar(roster));compare(std::move(h).finish(),hangars,"original hangar history");
  for(auto word:random.snapshot().words)require(word==r.integer<uint16_t>(),"original constructor random words");
  require(random.snapshot().first==r.integer<uint8_t>(),"original RNG first tap");require(random.snapshot().second==r.integer<uint8_t>(),"original RNG second tap");
  auto loaded{read_career(saved,game_edition::expansion)};
  for(auto &stock:loaded.inventories)for(auto &stack:stock)std::ranges::reverse(stack);
  compare(write_career(loaded),saved,"original save roundtrip");
  binary_reader cr{current};auto state{read_career_state(cr,game_edition::expansion,battle)};cr.require_end();compare(write_career_state(state),current,"selected state roundtrip");
 }
 auto unlocks{r.integer<uint32_t>()};
 for(unsigned i{};i<unlocks;++i){
  auto battle{r.integer<int16_t>()},missions{r.integer<int16_t>()},kind{r.integer<int16_t>()},grade{r.integer<int16_t>()};
  auto technology{read_technology(resources.read("zones","bst/tech_"+std::to_string(battle)+".dat"),game_edition::expansion,battle)};
  // The table loader keeps its mission-zero award; career initialization clears it separately.
  technology.last_type=technology.last_index=0;for(size_t j{};j<technology.entries.size();++j)if(technology.entries[j].available){technology.last_type=j>=9;technology.last_index=static_cast<int16_t>(j>=9?j-9:j==8?16:j);}
  advance_technology(technology,missions,kind,grade);
  require(technology.last_type==r.integer<int16_t>(),"technology award type "+std::to_string(i));require(technology.last_index==r.integer<int16_t>(),"technology award index "+std::to_string(i));
  for(size_t j{};j<technology.entries.size();++j){auto const &t{technology.entries[j]};auto early{r.integer<int16_t>()},late{r.integer<int16_t>()};auto available{r.integer<uint8_t>()};if(j<(battle?9:8)||(j>=9&&j<9+technology.weapon_count))require(t.early==early&&t.late==late&&t.available==available,"original technology entry "+std::to_string(i)+" slot "+std::to_string(j));}
 }
 r.require_end();std::cout<<count<<" complete original expansion career constructors and "<<unlocks<<" technology milestone/result checks agree\n";return 0;
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return 1;}
