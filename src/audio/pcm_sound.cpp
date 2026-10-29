#include "pcm_sound.h"
#include <algorithm>
#include <limits>

namespace earthsiege {
void pcm_sound_bank::bind(size_t const slot,bytes const samples){sounds.at(slot)={};sounds[slot].registered=true;sounds[slot].samples=samples;}
void pcm_sound_bank::set_master(uint8_t const volume){master=volume;}
void pcm_sound_bank::set_volume(size_t const slot,int8_t const volume){
  auto &s{sounds.at(slot)};s.volume=static_cast<uint8_t>(std::clamp<int>(volume,0,100));
  if(s.handle>=0 && channels.at(static_cast<size_t>(s.handle)).flags&0x8000)
    channels[static_cast<size_t>(s.handle)].volume=static_cast<uint16_t>(static_cast<uint32_t>(s.volume)*32767*master/10000);
}
void pcm_sound_bank::set_spatial_volume(size_t const slot,int8_t const volume){
  auto &s{sounds.at(slot)};s.spatial_volume=static_cast<uint16_t>(static_cast<int32_t>(volume)*32767*master/10000);
  if(s.handle>=0 && channels.at(static_cast<size_t>(s.handle)).flags&0x8000)channels[static_cast<size_t>(s.handle)].volume=s.spatial_volume;
}
void pcm_sound_bank::set_positioned(size_t const slot,bool const enabled,uint16_t const pan){auto &s{sounds.at(slot)};s.positional=enabled;s.pan=pan;}
void pcm_sound_bank::set_rate(size_t const slot,uint32_t const rate){
  auto &s{sounds.at(slot)};s.rate=rate<32768?32768:rate>0xf0000?(rate&65535)|0xf0000:rate;s.rate_override=true;
}
void pcm_sound_bank::set_loops(size_t const slot,int32_t const end,int32_t const start,uint16_t const count){
  auto &s{sounds.at(slot)};s.current_loops=count;
  if(playing(slot))channels[static_cast<size_t>(s.handle)].loops=count;
  else{s.loops=count;s.loop_start=start;s.loop_end=end;}
}
void pcm_sound_bank::allow_silent(size_t const slot,bool const enabled){sounds.at(slot).play_silent=enabled;}
bool pcm_sound_bank::playing(size_t const slot)const{auto const handle{sounds.at(slot).handle};return handle>=0 && static_cast<size_t>(handle)<channels.size() && (channels[static_cast<size_t>(handle)].flags&0x8000);}
uint32_t pcm_sound_bank::reported_volume(size_t const slot)const{
  auto const &s{sounds.at(slot)};if(!s.positional)return s.volume;
  // Original 0x5049c multiplies by 32767 after the division. Retain this
  // unusually large result in the stealing decision. Muting defines the
  // original divide-by-zero case without trapping the host process.
  return master?static_cast<uint32_t>(s.spatial_volume)*10000/master*32767:0;
}
int32_t pcm_sound_bank::allocate(pcm_start const &request){
  if(record)record->start(request);
  for(size_t i{};i<channels.size();++i)if(!(channels[i].flags&0x8000)){
    auto &v{channels[i]};v={};v.flags=static_cast<uint16_t>(request.flags|0xa000);v.volume=request.volume;v.loops=request.loops;v.key=static_cast<uint16_t>(request.slot+0x1000);
    v.rate=request.rate.value_or(65536);v.pan=request.pan.value_or(32768);v.loop_start=request.loop_start;v.loop_end=request.loop_end;v.samples=sounds[request.slot].samples;v.remaining=static_cast<uint32_t>(v.samples.size());return static_cast<int32_t>(i);
  }
  return -1;
}
int32_t pcm_sound_bank::start_voice(pcm_voice_state const &request){
  for(size_t i{};i<channels.size();++i)if(!(channels[i].flags&0x8000)){
    auto &v{channels[i]};v=request;v.flags|=0xa000;v.cursor=0;v.remaining=static_cast<uint32_t>(v.samples.size());v.consumed=0;v.output_cursor=0;v.drain_remaining=0;return static_cast<int32_t>(i);
  }return -1;
}
void pcm_sound_bank::stop_voice(size_t handle){auto &v{channels.at(handle)};if((v.flags&0x8000)&&!(v.flags&0x1000)){v.flags&=0x7fff;v.key=0;}}
void pcm_sound_bank::voice_pan(size_t handle,uint16_t pan){if(channels.at(handle).flags&0x8000)channels[handle].pan=pan;}
bool pcm_sound_bank::play(size_t const slot){
  auto &s{sounds.at(slot)};if(!s.registered)return false;
  if(std::count_if(channels.begin(),channels.end(),[](auto const &v){return v.flags&0x8000;})>30){
    std::optional<size_t> victim;uint32_t quietest{100};
    for(size_t i{};i<sounds.size();++i)if(sounds[i].registered && playing(i)){
      auto const gain{reported_volume(i)};if(gain<quietest){quietest=gain;victim=i;}
    }
    if(!victim)return false;
    stop(*victim);
  }
  auto const gain{s.positional?s.spatial_volume:static_cast<uint16_t>(static_cast<uint32_t>(s.volume)*32767*master/10000)};
  if(gain==0 && !s.play_silent)return true;
  pcm_start request{.slot=slot,.length=s.samples.size(),.loops=s.loops,.volume=gain,.flags=static_cast<uint16_t>(s.positional?0x4300:0x4100),.rate={},.pan={}};
  if(s.positional)request.pan=s.pan;
  if(s.rate_override)request.rate=s.rate;
  if(s.loops && s.loop_start!=-1 && s.loop_end!=-1){request.flags|=0x40;request.loop_start=s.loop_start;request.loop_end=s.loop_end;}
  s.current_loops=s.loops;s.marker=static_cast<uint16_t>(slot);s.handle=allocate(request);
  if(s.handle==-1){s.error=-1;s.marker=65535;return false;}
  return true;
}
void pcm_sound_bank::suspend(size_t const slot){auto const handle{sounds.at(slot).handle};if(handle>=0){if(record)record->stop(static_cast<size_t>(handle));auto &voice{channels.at(static_cast<size_t>(handle))};if((voice.flags&0x8000) && !(voice.flags&0x1000)){voice.flags&=0x7fff;voice.key=0;}}}
void pcm_sound_bank::stop(size_t const slot){suspend(slot);auto &s{sounds.at(slot)};s.handle=-1;s.marker=65535;}
void pcm_sound_bank::complete(size_t const handle){
  channels.at(handle).flags=0;
  for(auto &s:sounds)if(s.registered && s.handle==static_cast<int32_t>(handle)){s.handle=-1;s.marker=65535;break;}
  if(completed)completed(handle);
}
void pcm_sound_bank::voice_volume(size_t const handle,uint16_t const volume){if(channels.at(handle).flags&0x8000)channels[handle].volume=volume;}
pcm_sound_state const &pcm_sound_bank::sound(size_t const slot)const{return sounds.at(slot);}
std::array<pcm_voice_state,32> const &pcm_sound_bank::voices()const{return channels;}
} // namespace earthsiege
