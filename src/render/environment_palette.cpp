#include "environment_palette.h"
#include <algorithm>
#include <stdexcept>
namespace earthsiege {
namespace {
using colour=std::array<uint8_t,3>;
std::array<colour,16> ramp(colour const &first,colour const &last){
 std::array<colour,16> result;
 for(unsigned c{};c<3;++c){
  int const step{(static_cast<int>(last[c])-first[c])*256/15};
  for(int i{};i<15;++i)result[static_cast<size_t>(i)][c]=static_cast<uint8_t>(first[c]+((step*i)>>8));
  result[15][c]=last[c];
 }
 return result;
}
}
environment_palette::environment_palette(world_environment const &env)
 :indices{env.atmosphere_parameters[2],env.atmosphere_parameters[3]} {}
void environment_palette::update(palette &target,uint16_t pitch,bool enabled,bool impact){
 if(!enabled)return;
 if(!ready){
  base=target.dac_colours[64];impact_colour=target.dac_colours[98];
  endpoints=ramp(target.dac_colours.at(static_cast<size_t>(indices[0])),target.dac_colours.at(static_cast<size_t>(indices[1])));
  ready=true;
 }
 unsigned band{std::min(15U,static_cast<unsigned>(static_cast<uint16_t>(pitch+8192))>>10)};
 if(override_value>=0){band=static_cast<unsigned>(override_value);override_value=-1;}
 if(band>=16)throw std::out_of_range{"reflection band exceeds original palette"};
 auto const values{ramp(base,impact?impact_colour:endpoints[band])};
 for(size_t i{};i<values.size();++i){
  auto const &c{values[i]};target.dac_colours[64+i]=c;
  target.colours[64+i]=0xff000000U|(static_cast<uint32_t>(c[0]*255/63)<<16)|(static_cast<uint32_t>(c[1]*255/63)<<8)|static_cast<uint32_t>(c[2]*255/63);
 }
}
}
