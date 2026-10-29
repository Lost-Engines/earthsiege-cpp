#include "simulation/joystick_input.h"
#include <iostream>
using namespace earthsiege;
int main(int argc,char **argv)try{
 if(argc!=2)return 1;
 auto data{read_file(argv[1])};binary_reader f{data};auto magic{f.take(8)};if(std::string(magic.begin(),magic.end())!="ESJOY001")f.fail("fixture version");
 auto count{f.integer<uint32_t>()};
 for(unsigned row{};row<count;++row){
  joystick_profile p;p.kind=static_cast<joystick_kind>(f.integer<uint8_t>());p.response=f.integer<uint8_t>();p.axis_mask=f.integer<uint8_t>();p.button_mask=f.integer<uint8_t>();p.hat=1;
  auto buttons{f.integer<uint8_t>()};std::array<int32_t,4> raw;for(auto &v:raw)v=f.integer<int32_t>();
  auto c{read_joystick_calibration(f.take(64))};auto actual{sample_joystick(p,c,raw,buttons)};
  for(auto v:actual.axes)if(v!=f.integer<int32_t>())f.fail("joystick axis "+std::to_string(row));
  for(auto v:actual.buttons)if(v!=f.integer<uint8_t>())f.fail("joystick button "+std::to_string(row));
  for(auto v:actual.hat)if(v!=f.integer<uint8_t>())f.fail("joystick hat "+std::to_string(row));
  if(p.axis_mask!=f.integer<uint8_t>()||p.button_mask!=f.integer<uint8_t>()||p.hat!=f.integer<uint8_t>())f.fail("joystick metadata");
 }
 auto selections{f.integer<uint32_t>()};for(unsigned i{};i<selections;++i){
  auto model{f.integer<uint8_t>()};auto pedals{f.integer<uint8_t>()};auto profiles{joystick_profiles(model,pedals!=0)};auto n{f.integer<uint8_t>()};unsigned actual{};
  for(auto const &p:profiles)if(p.axis_mask){
   if(p.axis_mask!=f.integer<uint8_t>()||p.response!=f.integer<uint8_t>()||p.button_mask!=f.integer<uint8_t>()||p.hat!=f.integer<uint8_t>())f.fail("device selection "+std::to_string(i));
   ++actual;
  }
  if(actual!=n)f.fail("device count");
 }
 f.require_end();std::cout<<count<<" original joystick responses and "<<selections<<" device selections agree\n";return 0;
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return 1;}
