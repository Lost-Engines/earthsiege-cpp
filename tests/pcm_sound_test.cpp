#include "audio/pcm_sound.h"
#include <iostream>
#include <format>

using namespace earthsiege;
struct recorder:pcm_sound_bank::observer{
  std::vector<std::array<int64_t,11>> events;
  void start(pcm_start const &r)override{events.push_back({0,static_cast<int64_t>(r.slot),static_cast<int64_t>(r.length),r.loops,2,r.volume,r.flags,r.rate?static_cast<int64_t>(*r.rate):-1,r.pan?static_cast<int64_t>(*r.pan):-1,r.loop_start,r.loop_end});}
  void stop(size_t const handle)override{events.push_back({1,static_cast<int64_t>(handle),0,0,0,0,0,0,0,0,0});}
};
int main(int argc,char **argv)try{
  if(argc!=2)return 1;
  auto const data{read_file(argv[1])};binary_reader file{data};auto const magic{file.take(8)};if(std::string(magic.begin(),magic.end())!="ESPCMS01")file.fail("fixture version");
  std::array<std::vector<uint8_t>,60> samples;pcm_sound_bank bank;recorder recorded;bank.record=&recorded;
  for(size_t i{};i<samples.size();++i){samples[i].resize(1000+i*11);bank.bind(i,samples[i]);}
  auto const count{file.integer<uint32_t>()};
  for(uint32_t trial{};trial<count;++trial){
    auto const op{file.integer<uint8_t>()},slot{file.integer<uint8_t>()};auto const a{file.integer<uint32_t>()};auto const b{file.integer<int32_t>()},c{file.integer<int32_t>()};recorded.events.clear();int result{-1};
    switch(op){
      case 0:result=bank.play(slot);break;case 1:bank.stop(slot);break;case 2:bank.set_volume(slot,std::bit_cast<int8_t>(static_cast<uint8_t>(a)));break;
      case 3:bank.set_spatial_volume(slot,std::bit_cast<int8_t>(static_cast<uint8_t>(a)));break;case 4:bank.set_rate(slot,a);break;
      case 5:bank.set_loops(slot,std::bit_cast<int32_t>(a),b,static_cast<uint16_t>(c));break;case 6:bank.set_positioned(slot,a!=0,static_cast<uint16_t>(b));break;
      case 7:bank.complete(slot);break;case 8:bank.set_master(static_cast<uint8_t>(a));break;case 9:bank.suspend(slot);break;case 10:bank.allow_silent(slot,a!=0);break;
    }
    auto fail{[&](std::string const &what){throw std::runtime_error{std::format("case {} op {} slot {}: {}",trial,op,slot,what)};}};
    if(result!=file.integer<int32_t>())fail("play result");
    if(recorded.events.size()!=file.integer<uint32_t>())fail("event count");
    for(auto const &event:recorded.events)for(auto const value:event)if(value!=file.integer<int64_t>())fail("device event");
    for(size_t i{};i<60;++i){auto const &s{bank.sound(i)};std::array<int64_t,11>const actual{s.handle,s.marker,s.volume,s.spatial_volume,s.loops,s.current_loops,s.rate,s.rate_override,s.loop_start,s.loop_end,s.error};
      for(size_t field{};field<actual.size();++field){auto const expected{file.integer<int64_t>()};if(actual[field]!=expected)fail(std::format("sound {} field {}: {} != {}",i,field,actual[field],expected));}
    }
    for(auto const &v:bank.voices())for(auto const value:{v.flags,v.volume,v.loops,v.key})if(value!=file.integer<uint16_t>())fail("native voice state");
  }
  file.require_end();std::cout<<count<<" original PCM control, stealing and completion states match\n";return 0;
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return 1;}
