#include "audio/hmp_player.h"
#include "audio/music_loops.h"
#include <iostream>
#include <format>
int main(int argc,char **argv)try{
  using namespace earthsiege;if(argc!=2)return 1;auto const raw{read_file(argv[1])};binary_reader f{raw};auto const magic{f.take(8)};
  if(std::string(magic.begin(),magic.end())!="ESHMPLO1")f.fail("fixture version");
  auto const cases{f.integer<uint32_t>()};size_t packets{};
  for(uint32_t i{};i<cases;++i){auto const name{f.cstring()};auto const song{read_hmp(f.take(f.integer<uint32_t>()))};auto const repeats{f.integer<uint16_t>()};auto const ticks{f.integer<uint32_t>()};bool const playing{f.integer<uint8_t>()!=0};auto const remaining{f.integer<uint16_t>()};auto const count{f.integer<uint32_t>()};
    struct event {uint32_t tick; midi_packet packet;};std::vector<event> expected;
    for(uint32_t j{};j<count;++j){event e;e.tick=f.integer<uint32_t>();e.packet.device=f.integer<uint32_t>();auto const offset{f.integer<int32_t>()};if(offset>=0)e.packet.source_offset=static_cast<uint32_t>(offset);auto const wire{f.take(f.integer<uint8_t>())};e.packet.wire.assign(wire.begin(),wire.end());expected.push_back(std::move(e));}
    std::array<uint32_t,2> devices{0xa002,0xa005};hmp_player player{song,devices};music_loop_state loop{repeats,repeats};player.loop_filter=[&](uint8_t branch){return loop.allow(branch,player.playing());};size_t cursor{};
    for(uint32_t tick{1};tick<=ticks;++tick)for(auto const &p:player.tick()){
      if(cursor>=expected.size())throw std::runtime_error{name+" extra packet"};
      auto const &e{expected[cursor++]};
      if(tick!=e.tick||p.device!=e.packet.device||p.wire!=e.packet.wire||p.source_offset!=e.packet.source_offset)throw std::runtime_error{std::format("{} repeat {} tick {} packet {}",name,repeats,tick,cursor)};
      ++packets;
    }
    if(cursor!=count||player.playing()!=playing||loop.remaining!=remaining)throw std::runtime_error{name+" loop result"};
  }f.require_end();std::cout<<cases<<" original complete-song loop cases, "<<packets<<" timed packets match\n";return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}
