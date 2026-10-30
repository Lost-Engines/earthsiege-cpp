#include "shell/mission_preferences.h"
#include <iostream>
using namespace earthsiege;
int main(int argc,char **argv)try{
 if(argc!=2)return 1;
 auto data{read_file(argv[1])};binary_reader f{data};auto magic{f.take(8)};
 if(std::string(magic.begin(),magic.end())!="ESPRFP01")f.fail("fixture version");
 auto count{f.integer<uint32_t>()};unsigned total{};
 for(unsigned i{};i<count;++i){
  preferences p{f.take(17)};bool radio{f.integer<uint8_t>()!=0};auto actions{f.integer<uint16_t>()};mission_preferences state{p,radio};
  auto check{[&]{for(auto const *values:{&p.current(),&p.saved(),&p.previous(),&state.persisted()})for(auto v:*values)if(f.integer<uint8_t>()!=v)f.fail("preference transaction mismatch");}};
  check();
  for(unsigned j{};j<actions;++j){auto index{f.integer<uint8_t>()};bool reverse{f.integer<uint8_t>()!=0};auto result{state.activate(index,reverse)};if((result==mission_preference_action::done)!=(index==8))f.fail("Done action");check();++total;}
 }
 f.require_end();std::cout<<total<<" original in-flight preference changes agree\n";return 0;
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return 1;}
