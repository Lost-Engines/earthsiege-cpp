#include "audio/pcm_mixer.h"
#include <format>
#include <iostream>

int main(int argc,char **argv)try{
  using namespace earthsiege;
  if(argc!=3)return 1;
  auto const kernels{read_file(argv[1])};binary_reader input{kernels};input.take(8);auto const count{input.integer<uint32_t>()};
  for(uint32_t i{};i<count;++i){
    bool const stereo{input.integer<uint8_t>()!=0};auto const volume{input.integer<uint16_t>()};auto const pan{input.integer<int32_t>()};auto const source{input.take(256)};size_t const channels{stereo?2u:1u};auto const background{input.take(256*channels)},expected{input.take(256*channels)};
    for(size_t j{};j<256;++j)for(size_t channel{};channel<channels;++channel){auto const balance{pan>=0 && stereo?std::min(32767,channel?pan:65535-pan):32767};auto const at{j*channels+channel};if(mix_pcm8(source[j],background[at],volume,static_cast<uint16_t>(balance))!=expected[at])throw std::runtime_error{std::format("original mixer kernel {} sample {}",i,at)};}
  }
  input.require_end();auto const traces{read_file(argv[2])};binary_reader trace{traces};trace.take(8);auto const trials{trace.integer<uint32_t>()};size_t states{};
  for(uint32_t trial{};trial<trials;++trial){
    bool const stereo{trace.integer<uint8_t>()!=0};auto const length{trace.integer<uint32_t>()};auto const loops{trace.integer<uint16_t>()},volume{trace.integer<uint16_t>()};auto const pan{trace.integer<int32_t>()};auto const rate{trace.integer<uint32_t>()};auto const seed{trace.integer<uint8_t>()};auto const steps{trace.integer<uint32_t>()};
    std::vector<uint8_t> samples(length);for(size_t i{};i<samples.size();++i)samples[i]=static_cast<uint8_t>(i*7+seed);
    pcm_sound_bank bank;bank.bind(0,samples);bank.set_master(100);bank.allow_silent(0,true);bank.set_rate(0,rate);bank.set_loops(0,-1,-1,loops);
    // This fixture starts at the HMI descriptor boundary, so its arbitrary
    // 15-bit volume is independent of the separately tested percent setter.
    bank.set_positioned(0,pan>=0,static_cast<uint16_t>(std::max(pan,0)));bank.play(0);
    // Descriptor injection is exposed as an explicit digital MIDI/backend API.
    bank.voice_volume(0,volume);
    pcm_mixer mixer{bank,stereo};
    for(uint32_t step{};step<steps;++step){
      mixer.advance(trace.integer<uint16_t>());auto const &v{bank.voices()[0]};std::array<uint32_t,8>const actual{v.flags,v.volume,v.loops,v.cursor,v.remaining,v.consumed,v.output_cursor,v.drain_remaining};
      for(size_t field{};field<actual.size();++field){auto const expected{trace.integer<uint32_t>()};if(expected!=actual[field])throw std::runtime_error{std::format("trial {} step {} field {}: {} != {}",trial,step,field,actual[field],expected)};}
      if(crc32(mixer.ring())!=trace.integer<uint32_t>())throw std::runtime_error{std::format("trial {} step {} DMA buffer",trial,step)};
      ++states;
    }
  }
  trace.require_end();std::cout<<count<<" original sample kernels and "<<states<<" DMA/loop/release states match\n";return 0;
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return 1;}
