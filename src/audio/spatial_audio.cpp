#include "spatial_audio.h"
#include <algorithm>

namespace earthsiege {
positional_audio_state spatial_audio_gain(vec3<int32_t> const &position,model_transform const &listener,uint8_t const base,int32_t cutoff,unsigned int shift,uint16_t const previous_pan,bool const enabled){
  if(!enabled)return {{},previous_pan};
  if(cutoff<=65535)shift=0;
  shift&=31;cutoff>>=shift;
  vec3<int32_t> delta{wrap_long(static_cast<int64_t>(position.x)-listener.translation.x)>>shift,wrap_long(static_cast<int64_t>(position.y)-listener.translation.y)>>shift,wrap_long(static_cast<int64_t>(position.z)-listener.translation.z)>>shift};
  auto const distance{std::max(1,fast_distance(delta))};
  if(distance>=cutoff)return {uint8_t{0},previous_pan};
  auto const gain{static_cast<uint8_t>(base-wrap_long(static_cast<int64_t>(distance)*base)/cutoff)};
  vec3<int32_t> unit{wrap_long(static_cast<int64_t>(delta.x)*4095)/distance,wrap_long(static_cast<int64_t>(delta.y)*4095)/distance,wrap_long(static_cast<int64_t>(delta.z)*4095)/distance};
  auto const right{rotate_vector({4095,0,0},listener)};
  auto const dot{wrap_long(static_cast<int64_t>(unit.x)*right.x+static_cast<int64_t>(unit.y)*right.y+static_cast<int64_t>(unit.z)*right.z)};
  return {gain,static_cast<uint16_t>(65535-(dot/4095+4095)*8)};
}
void sound_sources::submit(size_t const id,vec3<int32_t> const &position,vec3<int32_t> const &listener,bool const enabled){
  if(!enabled || pending.at(id)!=0)return;
  pending[id]=fast_distance({wrap_long(static_cast<int64_t>(listener.x)-position.x),wrap_long(static_cast<int64_t>(listener.y)-position.y),wrap_long(static_cast<int64_t>(listener.z)-position.z)});positions[id]=position;
}
std::vector<positional_sound> sound_sources::flush(vec3<int32_t> const &listener,bool const enabled){
  std::vector<positional_sound> result;if(!enabled)return result;
  for(size_t i{};i<pending.size();++i)if(pending[i]){result.push_back({i,vec3<int32_t>{positions[i]}});pending[i]=0;}
  if(--cadence==0){cadence=10;origin=listener;}
  return result;
}
unsigned int sound_sources::remaining()const{return cadence;}
vec3<int32_t> sound_sources::listener()const{return vec3<int32_t>{origin};}
std::array<int32_t,60> const &sound_sources::distances()const{return pending;}
std::array<vec3<int32_t>,60> const &sound_sources::sources()const{return positions;}
} // namespace earthsiege
