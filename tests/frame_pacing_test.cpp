#include "simulation/frame_clock.h"
#include "resources/binary_reader.h"
#include <iostream>
using namespace earthsiege;
int main(int argc,char **argv)try{
 if(argc!=2)return 1;
 auto data{read_file(argv[1])};binary_reader f{data};auto magic{f.take(8)};
 if(std::string(magic.begin(),magic.end())!="ESPACE01")f.fail("fixture version");
 auto traces{f.integer<uint32_t>()},steps{f.integer<uint32_t>()};
 for(unsigned t{};t<traces;++t){
  frame_clock_state initial;initial.maximum=f.integer<int16_t>();initial.nominal=f.integer<int16_t>();initial.alternate_nominal=f.integer<int16_t>();
  initial.fast=f.integer<uint32_t>();initial.slow=f.integer<uint32_t>();initial.countdown=initial.maximum;initial.elapsed=initial.nominal;frame_clock clock{initial};
  for(unsigned s{};s<steps;++s){
   clock.advance(f.integer<uint16_t>());auto expected{f.integer<uint8_t>()!=0};
   if(clock.ready()!=expected||clock.take().has_value()!=expected)f.fail("pacing gate");
   auto const &v{clock.state()};
   for(auto word:{v.countdown,v.elapsed,v.ratio,v.alternate_ratio})if(word!=f.integer<int16_t>())f.fail("frame ratio/counter");
   for(auto word:{v.fast,v.slow})if(word!=f.integer<uint32_t>())f.fail("clock wrap");
  }
 }
 f.require_end();std::cout<<traces*steps<<" original timer/pacing states agree\n";return 0;
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return 1;}
