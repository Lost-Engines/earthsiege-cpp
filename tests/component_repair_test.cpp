#include "shell/component_repair.h"
#include "resources/resource_store.h"
#include <iostream>
#include <stdexcept>
using namespace earthsiege;
namespace {
herc_condition conditions(binary_reader &f){
 herc_condition c;for(auto &v:c.armour)v=f.integer<int16_t>();for(auto &v:c.internal)v=f.integer<int16_t>();for(auto &v:c.equipment)v=f.integer<int16_t>();return c;
}
}
int main(int argc,char **argv)try{
 if(argc!=3)return 1;
 resource_store r{argv[1]};auto prices{read_service_prices(r.read("shell0","cfg/damage.dat"))};
 auto data{read_file(argv[2])};binary_reader f{data};auto magic{f.take(8)};
 if(std::string(magic.begin(),magic.end())!="ESREPR01")throw std::runtime_error{"fixture version"};
 auto cases{f.integer<uint32_t>()};unsigned events{};
 for(unsigned sequence{};sequence<cases;++sequence){
  player_herc h;career_state c;h.chassis=f.integer<int16_t>();auto count{f.integer<int16_t>()};
  for(int i{};i<count;++i)h.weapons.push_back(f.integer<int16_t>());
  c.budget=c.crew[0].budget_share=f.integer<int32_t>();h.condition=conditions(f);
  component_repair edit{h,c,prices};
  auto compare=[&](){
   auto budget{f.integer<int32_t>()},share{f.integer<int32_t>()};
   if(c.budget!=budget||c.crew[0].budget_share!=share){std::cerr<<"sequence "<<sequence<<" event "<<events<<" budget "<<c.budget<<" != "<<budget<<'\n';throw std::runtime_error{"repair budget"};}
   std::array<int16_t,5> actual{static_cast<int16_t>(edit.group()),edit.index(),edit.allocated(),edit.tier(),edit.required()};
   for(size_t i{};i<actual.size();++i){auto expected{f.integer<int16_t>()};if(actual[i]!=expected){std::cerr<<"sequence "<<sequence<<" event "<<events<<" field "<<i<<": "<<actual[i]<<" != "<<expected<<'\n';throw std::runtime_error{"repair state"};}}
   auto expected{conditions(f)};if(h.condition.armour!=expected.armour||h.condition.internal!=expected.internal||h.condition.equipment!=expected.equipment){std::cerr<<"sequence "<<sequence<<" event "<<events<<'\n';throw std::runtime_error{"repair conditions"};}
  };
  compare();auto count_events{f.integer<uint16_t>()};
  for(unsigned i{};i<count_events;++i){
   auto action{f.integer<int16_t>()},index{f.integer<int16_t>()};
   if(action==106)edit.increase(h,c);
   else if(action==107)edit.decrease(h,c);
   else if(action>=1924&&action<=1926)edit.select(h,static_cast<repair_group>(action),0);
   else if(action==1921)edit.select(h,edit.group(),index);
   else if(action==1900)edit.accept(h,c);
   else if(action==99)edit.cancel(h,c);
   ++events;compare();
  }
 }
 f.require_end();std::cout<<cases<<" native component-repair sequences, "<<events<<" events agree\n";return 0;
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return 1;}
