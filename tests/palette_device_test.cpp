#include "render/palette_device.h"
#include <iostream>
using namespace earthsiege;
int main(int argc,char **argv)try{
 if(argc!=2)return 1;
 auto raw=read_file(argv[1]);binary_reader f{raw};auto magic=f.take(8);
 if(std::string(magic.begin(),magic.end())!="ESDAC001")f.fail("palette fixture");
 std::array<palette,2> sources;
 for(auto &p:sources)for(auto &c:p.dac_colours)for(auto &v:c)v=f.integer<uint8_t>();
 palette_device display;auto count=f.integer<uint32_t>();
 for(unsigned i{};i<count;++i){
  auto source=f.integer<uint8_t>(),changes=f.integer<uint8_t>();
  auto &p=sources.at(source);
  for(unsigned j{};j<changes;++j){
   auto first=f.integer<uint16_t>(),n=f.integer<uint16_t>();
   for(unsigned k{first};k<first+n;++k)for(auto &v:p.dac_colours.at(k))v=f.integer<uint8_t>();
   display.changed(first,n);
  }
  display.present(p);
  for(auto const &c:display.colours().dac_colours)for(auto v:c)
   if(v!=f.integer<uint8_t>())f.fail("DAC component at step "+std::to_string(i));
  if(display.next_index()!=f.integer<uint8_t>())f.fail("DAC cursor");
 }
 f.require_end();std::cout<<count<<" original palette-manager/DAC states match\n";return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}
