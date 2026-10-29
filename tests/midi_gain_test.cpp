#include "audio/midi_gain.h"
#include <iostream>
#include <algorithm>
#include <format>
int main(int argc,char **argv)try{
  using namespace earthsiege;if(argc!=2)return 1;auto const raw{read_file(argv[1])};binary_reader f{raw};auto const magic{f.take(8)};
  if(std::string(magic.begin(),magic.end())!="ESMGain1")f.fail("fixture version");
  std::vector<std::pair<size_t,uint8_t>> tracks(f.integer<uint8_t>());for(auto &[d,ch]:tracks){d=f.integer<uint8_t>();ch=f.integer<uint8_t>();}
  auto const count{f.integer<uint32_t>()};std::array<uint32_t,2> ids{0,1};midi_gain gain{ids};uint8_t song{127};size_t packets{};
  for(uint32_t i{};i<count;++i){auto const op{f.integer<uint8_t>()},value{f.integer<uint8_t>()},device{f.integer<uint8_t>()};auto const msg{f.take(f.integer<uint8_t>())};std::vector<midi_packet> actual;
    if(op==0)actual.push_back(gain.apply({device,{msg.begin(),msg.end()},{}},song));else if(op==1)actual=gain.set_master(value);else{song=value;actual=gain.set_song(value,tracks);}
    auto const n{f.integer<uint16_t>()};if(n!=actual.size())throw std::runtime_error{std::format("gain {} packet count {} / {}",i,actual.size(),n)};
    for(auto const &p:actual){auto const d{f.integer<uint8_t>()};auto const wire{f.take(f.integer<uint8_t>())};if(d!=p.device||!std::ranges::equal(wire,p.wire))throw std::runtime_error{std::format("gain {} packet",i)};++packets;}
    for(auto const &vol:gain.volumes())for(auto const v:vol)if(v!=f.integer<uint8_t>())throw std::runtime_error{"channel volume"};
    if(gain.master()!=f.integer<uint8_t>()||song!=f.integer<uint32_t>())throw std::runtime_error{"gain state"};
  }f.require_end();std::cout<<count<<" original gain updates and "<<packets<<" physical MIDI packets match\n";return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}
