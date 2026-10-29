#include "shell/control_settings.h"
#include <iostream>
using namespace earthsiege;
int main(int argc,char **argv)try{
 if(argc!=2)return 1;
 auto raw{read_file(argv[1])};binary_reader f{raw};auto magic{f.take(8)};
 if(std::string(magic.begin(),magic.end())!="ESCTLP01")f.fail("fixture version");auto count{f.integer<uint32_t>()};unsigned steps{};
 for(unsigned c{};c<count;++c){
  preferences prefs{f.take(17)};control_settings_state controls{prefs};auto events{f.integer<uint16_t>()};
  auto compare{[&](unsigned step){
   for(auto v:controls.state())if(v!=f.integer<uint8_t>())f.fail("control local state "+std::to_string(c)+"/"+std::to_string(step));
   for(auto v:prefs.current())if(v!=f.integer<uint8_t>())f.fail("control preferences "+std::to_string(c)+"/"+std::to_string(step));
   for(auto v:prefs.saved())if(v!=f.integer<uint8_t>())f.fail("control accepted preferences "+std::to_string(c)+"/"+std::to_string(step));
   for(auto v:controls.persisted())if(v!=f.integer<uint8_t>())f.fail("control persisted preferences "+std::to_string(c)+"/"+std::to_string(step));
  }};
  compare(0);
  for(unsigned i{};i<events;++i){auto action{f.integer<uint8_t>()};bool const reverse{f.integer<uint8_t>()!=0},keyboard{f.integer<uint8_t>()!=0};
   controls.activate(action-1,reverse,keyboard);compare(i+1);++steps;}
 }
 f.require_end();std::cout<<count<<" original controls sequences / "<<steps<<" changes agree\n";return 0;
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return 1;}
