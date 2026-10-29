#include "hmi_pcm.h"
#include <stdexcept>

namespace earthsiege {
hmi_pcm::hmi_pcm(bytes bank,pcm_midi_backend &output):backend{output}{
  for(auto &slot:slots)slot.fill(-1);
  binary_reader r{bank,"HMI digital bank"};auto const signature{r.take(8)};
  if(std::string(signature.begin(),signature.end())!="HMIDIGIP")r.fail("signature");
  r.seek(36);auto const end{r.integer<uint32_t>()};if(end>bank.size())r.fail("bank size");
  while(r.position()<end){auto const at{r.position()};std::array<uint32_t,29> d{};for(auto &word:d)word=r.integer<uint32_t>();
    auto const pcm{r.take(d[2])};if(d[6]>=128)r.fail("note number");if(d[10]&0x40)r.fail("unsupported subrange-loop percussion bank");
    samples[d[6]]=midi_sample{d[6],d[5],d[16],d[10],static_cast<uint32_t>(at+116),std::bit_cast<int32_t>(d[9]),static_cast<uint16_t>(d[4]),pcm};
  }
  if(r.position()!=end)r.fail("bank endpoint");
}
auto hmi_pcm::remove(std::optional<int32_t> handle,std::optional<uint8_t> note,std::optional<uint8_t> channel)->int32_t {
  if(!count_)return -1;
  uint32_t index{};for(;index<32;++index){auto const &s{slots[index]};if(s[0]!=-1&&(!handle||s[0]==*handle)&&(!note||s[1]==*note)&&(!channel||s[3]==*channel))break;}
  if(index==32)return -1;
  auto const result{slots[index][0]};
  while(index!=tail_){auto const previous{(index-1)&31};slots[index]=slots[previous];index=previous;}
  slots[index].fill(-1);tail_=(tail_+1)&31;--count_;return result;
}
void hmi_pcm::complete(int32_t handle){remove(handle,{},{});}
void hmi_pcm::midi(bytes msg){
  if(msg.empty())return;
  auto const type{msg[0]&0xf0};auto const channel{static_cast<uint8_t>(msg[0]&15)};
  if(type!=0x80&&type!=0x90&&type!=0xb0)return;
  if(msg.size()<3)throw std::runtime_error{"Truncated PCM MIDI packet"};
  auto stop=[&](std::optional<uint8_t> note){auto const handle{remove({},note,channel)};if(handle!=-1)backend.stop(handle);return handle;};
  if(type==0x80||type==0x90){
    if(msg[1]>=samples.size() || !samples[msg[1]])return;
    auto &s{*samples[msg[1]]};
    if(s.note_off_marker!=-32768)stop(msg[1]);
    if(type==0x80||!msg[2])return;
    if(count_>=8)stop({});
    s.volume=((volume_*msg[2])>>7)<<8;if(!(pan_&128))s.pan=pan_<<9;
    auto const handle{backend.start(s)};
    if(handle!=-1&&count_<8){slots[head_]={handle,msg[1],msg[2],channel};head_=(head_+1)&31;++count_;}
  }else if(msg[1]==123){while(stop({})!=-1){}}
  else if(msg[1]==7){volume_=msg[2];for(auto i{tail_};i!=head_;i=(i+1)&31)if(slots[i][0]!=-1)backend.volume(slots[i][0],volume_*static_cast<uint32_t>(slots[i][2])*2);}
  else if(msg[1]==10){pan_=msg[2];for(auto i{tail_};i!=head_;i=(i+1)&31)backend.pan(slots[i][0],pan_<<9);}
}
}
