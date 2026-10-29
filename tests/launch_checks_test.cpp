#include "shell/launch_checks.h"
#include "resources/resource_store.h"
#include <iostream>
#include <stdexcept>
using namespace earthsiege;
int main(int argc,char **argv)try{
 if(argc!=3)return 1;
 resource_store r{argv[1]};auto original{read_hangar(r.read("shell0","car/sm_init.dat"))};
 auto bytes{read_file(argv[2])};binary_reader f{bytes};auto magic{f.take(8)};
 if(std::string(magic.begin(),magic.end())!="ESREADY1")throw std::runtime_error{"fixture version"};
 auto cases{f.integer<uint32_t>()};
 for(unsigned sequence{};sequence<cases;++sequence){
  auto h{original};career_state c;auto mask{f.integer<uint16_t>()};
  for(auto &p:c.crew)p.hangar_slot=f.integer<int16_t>();
  for(auto &p:c.crew)p.active=f.integer<uint8_t>();
  c.selected_crew=f.integer<int16_t>();c.crew_cursor=f.integer<int16_t>();
  h.selected=f.integer<int16_t>();h.construction_slot=f.integer<int16_t>();
  for(auto &p:h.hercs){auto weapons{f.integer<uint16_t>()};for(size_t j{};j<p.weapons.size();++j)p.weapons[j]=weapons>>j&1?static_cast<int16_t>(j%17):17;}
  for(auto &p:h.hercs)for(auto index:{0,1,5})p.condition.internal[static_cast<size_t>(index)]=f.integer<int16_t>();
  std::erase_if(h.hercs,[mask](auto const &p){return !(mask>>p.slot&1);});
  for(int action{};action<4;++action){
   auto copy{h};auto crew{c};unsigned warning{action<2?launch_warning(copy,crew):action==2?next_mission_warning(copy,crew):construction_available(copy)?0u:7772u};
   unsigned const expected{f.integer<uint16_t>()};auto cursor{f.integer<int16_t>()},selection{f.integer<int16_t>()};
   unsigned const result{warning?warning:action==0?1700u:action==1?9999u:action==2?1499u:2000u};
   if(expected!=result||cursor!=crew.crew_cursor||selection!=copy.selected){
    std::cerr<<"case "<<sequence<<" action "<<action<<" got "<<result<<','<<crew.crew_cursor<<','<<copy.selected<<" expected "<<expected<<','<<cursor<<','<<selection<<'\n';
    throw std::runtime_error{"original launch check"};
   }
  }
 }
 f.require_end();std::cout<<cases*4<<" original bay/map launch checks agree\n";return 0;
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return 1;}
