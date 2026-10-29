#include "simulation/joystick_routing.h"
#include <iostream>
using namespace earthsiege;
int main(int argc,char **argv)try{
 if(argc!=2)return 1;
 auto raw{read_file(argv[1])};binary_reader f{raw};auto magic{f.take(8)};
 if(std::string(magic.begin(),magic.end())!="ESJOYRT1")f.fail("fixture version");
 auto count{f.integer<uint32_t>()};
 for(unsigned row{};row<count;++row){
  auto b{[&]{return f.integer<uint8_t>()!=0;}};
  device_routing_options o{f.integer<uint8_t>(),f.integer<uint8_t>(),b(),b(),b(),b(),b(),b(),b(),b()};
  joystick_sample keys,device;
  for(auto &v:keys.axes)v=f.integer<int32_t>();
  for(auto &v:keys.buttons)v=f.integer<uint8_t>();
  for(auto &v:device.axes)v=f.integer<int32_t>();
  for(auto &v:device.buttons)v=f.integer<uint8_t>();
  for(auto &v:device.hat)v=f.integer<uint8_t>();
  auto throttle{f.integer<int16_t>()};auto actual{route_devices(o,keys,device,throttle)};
  unsigned field{};
  for(auto v:actual.axes){auto expected{f.integer<int16_t>()};if(v!=expected)f.fail("routing axis "+std::to_string(row)+"/"+std::to_string(field)+" "+std::to_string(v)+" != "+std::to_string(expected));++field;}
  for(auto v:actual.buttons)if(v!=f.integer<uint8_t>())f.fail("routing button "+std::to_string(row));
  for(auto v:actual.hat)if(v!=f.integer<uint8_t>())f.fail("routing hat "+std::to_string(row));
  if((actual.aiming_camera?17:13)!=f.integer<uint8_t>())f.fail("routing camera alias");
  if(o.mode!=f.integer<uint8_t>())f.fail("routing mode "+std::to_string(row));
  if(o.switch_held!=b())f.fail("routing switch latch");
 }
 f.require_end();std::cout<<count<<" original mixed joystick/keyboard routes agree\n";return 0;
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return 1;}
