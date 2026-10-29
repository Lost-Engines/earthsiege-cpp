#include "pcm_mixer.h"
#include <algorithm>
#include <bit>
#include <stdexcept>

namespace earthsiege {
uint8_t mix_pcm8(uint8_t const sample,uint8_t const background,uint16_t const volume,uint16_t const balance){
  auto const product{static_cast<int32_t>(std::bit_cast<int16_t>(volume))*std::bit_cast<int16_t>(balance)};
  auto const gain{std::bit_cast<int8_t>(static_cast<uint8_t>(((product>>16)*2)>>8))};
  auto const scaled{std::bit_cast<int8_t>(static_cast<uint8_t>(((static_cast<int>(sample)-128)*gain)>>7))};
  return static_cast<uint8_t>(std::clamp(static_cast<int>(scaled)+background-128,-128,127)+128);
}
pcm_mixer::pcm_mixer(pcm_sound_bank &sounds,bool const stereo_output):bank{sounds},stereo{stereo_output}{buffer.fill(128);}
std::vector<uint8_t> pcm_mixer::advance(uint16_t const byte_count){
  if(byte_count>=buffer.size())throw std::invalid_argument{"PCM DMA step exceeds ring capacity"};
  std::vector<uint8_t> output;output.reserve(byte_count);
  for(uint32_t i{};i<byte_count;++i)output.push_back(buffer[(dma_position+i)%buffer.size()]);
  dma_position=(dma_position+byte_count)%static_cast<uint32_t>(buffer.size());
  uint32_t position{dma_position&~3u};if(position==0)position=32;
  uint32_t const elapsed{position>=previous_position?position-previous_position:static_cast<uint32_t>(buffer.size())-previous_position+position};
  uint32_t const fill_count{elapsed+((elapsed>>3)&~3u)};
  uint32_t lead{position+(stereo?128u:256u)};if(lead>buffer.size())lead-=static_cast<uint32_t>(buffer.size());
  std::vector<size_t> active;
  for(size_t i{};i<bank.channels.size();++i)if((bank.channels[i].flags&0x8000) && !(bank.channels[i].flags&0x1000))active.push_back(i);
  for(size_t i{};i<draining.size();){
    auto const slot{draining[i]};auto &v{bank.channels[slot]};
    if(v.drain_remaining<elapsed){
      v.drain_remaining=0;auto const flags{static_cast<uint16_t>(v.flags&0x6fff)};bank.complete(slot);v.flags=flags;draining.erase(draining.begin()+static_cast<std::ptrdiff_t>(i));
    }else{v.drain_remaining=static_cast<uint16_t>(v.drain_remaining-elapsed);++i;}
  }
  for(uint32_t i{};i<elapsed;++i)buffer[(previous_position+i)%buffer.size()]=128;
  for(auto const slot:active){
    auto &v{bank.channels[slot]};
    if(v.flags&0x2000){v.flags&=0xdfff;v.output_cursor=static_cast<uint16_t>(lead);}
    uint32_t count{fill_count};
    if(v.remaining<=65535)count=std::min(count,v.remaining*(stereo?2u:1u));
    auto const available{previous_position>=v.output_cursor?previous_position-v.output_cursor:static_cast<uint32_t>(buffer.size())-v.output_cursor+previous_position};
    count=std::min(count,available);
    auto const frames{count>>(stereo?1:0)};
    auto const pan{(v.flags&0x200)!=0};
    std::array<uint16_t,2>const balance{static_cast<uint16_t>(pan&&stereo?std::min<unsigned int>(32767,65535-v.pan):32767),static_cast<uint16_t>(pan&&stereo?std::min<unsigned int>(32767,v.pan):32767)};
    uint32_t write{v.output_cursor};
    for(uint32_t i{};i<frames;++i){
      auto const sample{v.cursor+i<v.samples.size()?v.samples[v.cursor+i]:uint8_t{128}};
      for(unsigned int channel{};channel<(stereo?2u:1u);++channel){if(write==buffer.size())write=0;buffer[write]=mix_pcm8(sample,buffer[write],v.volume,balance[channel]);++write;}
    }
    v.output_cursor=static_cast<uint16_t>(write);v.cursor+=frames;v.remaining-=frames;v.consumed+=frames;
    // The original stops/restarts at fewer than four remaining source bytes.
    if(static_cast<int32_t>(v.remaining)<4){
      if((v.flags&0x4000) && v.loops){
        if(v.loops!=65535)--v.loops;
        if(v.flags&0x40){
          // No shipped SoundManager descriptor enables subrange loops. Bound
          // manually supplied regions instead of reading its unset tail word.
          v.cursor=static_cast<uint32_t>(std::clamp<int32_t>(v.loop_start,0,static_cast<int32_t>(v.samples.size())));
          v.remaining=static_cast<uint32_t>(std::clamp<int32_t>(v.loop_end,0,static_cast<int32_t>(v.samples.size()-v.cursor)));
        }else{v.cursor=0;v.remaining=static_cast<uint32_t>(v.samples.size());}
      }else{
        v.flags|=0x1000;draining.push_back(slot);
        v.drain_remaining=static_cast<uint16_t>(v.output_cursor>=position?v.output_cursor-position:buffer.size()-position+v.output_cursor);
      }
    }
  }
  previous_position=position;return output;
}
std::array<uint8_t,4096> const &pcm_mixer::ring()const{return buffer;}
} // namespace earthsiege
