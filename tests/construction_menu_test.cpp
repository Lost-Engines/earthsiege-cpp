#include "shell/construction_menu.h"
#include <iostream>
#include <stdexcept>
using namespace earthsiege;
int main(int argc,char **argv)try{
 if(argc!=3)return 1;
 resource_store r{argv[1]};armament_catalog catalog{r};auto prices{read_service_prices(r.read("shell0","cfg/damage.dat"))};
 auto data{read_file(argv[2])};binary_reader f{data};auto magic{f.take(8)};
 if(std::string(magic.begin(),magic.end())!="ESBUILD2")throw std::runtime_error{"fixture version"};
 auto cases{f.integer<uint32_t>()};unsigned events{};
 for(unsigned sequence{};sequence<cases;++sequence){
  auto mask{f.integer<int16_t>()},chosen{f.integer<int16_t>()};career_state c;c.budget=f.integer<int32_t>();hangar h;armory inventory;
  for(int16_t i{1};i<8;++i){player_herc p;p.slot=i;h.hercs.push_back(p);}
  for(size_t i{};i<8;++i)c.technology.entries[i].available=(mask>>i)&1;
  auto remaining{f.integer<int16_t>()};auto value{f.integer<int32_t>()};
  if(remaining){auto const budget{c.budget};reserve_construction(h);construct_herc(h,c,chosen,catalog.mounts(chosen).size(),prices);h.construction_remaining=remaining;h.construction_value=value;c.budget=budget;}
  construction_menu menu{h,c,prices,chosen};
  auto compare=[&](){
   auto ch{f.integer<int16_t>()};auto budget{f.integer<int32_t>()};
   std::array<int16_t,4> actual{static_cast<int16_t>(h.hercs.size()),h.selected,h.construction_slot,h.construction_remaining};
   bool ok{ch==menu.chassis()&&c.budget==budget};
   for(auto actual_value:actual){auto expected{f.integer<int16_t>()};ok=ok&&actual_value==expected;}
   auto expected_value{f.integer<int32_t>()};auto label{f.integer<int16_t>()};
   std::array<int16_t,3> disabled{static_cast<int16_t>(menu.enabled()?0:2),static_cast<int16_t>(menu.cycling()?0:2),static_cast<int16_t>(menu.cycling()?0:2)};
   ok=ok&&expected_value==h.construction_value&&label==(menu.building(h)?205:206);
   for(auto v:disabled){auto expected{f.integer<int16_t>()};ok=ok&&v==expected;}
   if(!ok){std::cerr<<"sequence "<<sequence<<" event "<<events<<'\n';throw std::runtime_error{"native construction menu"};}
  };
  compare();auto count{f.integer<uint16_t>()};
  for(unsigned i{};i<count;++i){auto action{f.integer<int16_t>()};if(action==2001)menu.activate(h,c,inventory,catalog);else menu.cycle(c,action==2004?1:-1);++events;compare();}
 }
 f.require_end();std::cout<<cases<<" native construction menu sequences, "<<events<<" events agree\n";return 0;
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return 1;}
