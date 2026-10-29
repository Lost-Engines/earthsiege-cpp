#include "audio/hmp_player.h"
#include <iostream>
#include <format>
int main(int argc,char **argv)try{
  using namespace earthsiege;if(argc!=2)return 1;auto const data{read_file(argv[1])};binary_reader f{data};auto const magic{f.take(8)};
  if(std::string(magic.begin(),magic.end())!="ESHMP001")f.fail("fixture version");
  auto const count{f.integer<uint32_t>()};size_t packets{},deltas{},routes{};
  for(uint32_t song_index{};song_index<count;++song_index){
    auto const name{f.cstring()};auto const raw{f.take(f.integer<uint32_t>())};auto const song{read_hmp(raw)};auto const delta_count{f.integer<uint32_t>()};
    for(uint32_t d{};d<delta_count;++d){auto const offset{f.integer<uint32_t>()},expected{f.integer<uint32_t>()};binary_reader r{raw};r.seek(offset);if(read_hmp_delta(r)!=expected)throw std::runtime_error{name+" delta"};++deltas;}
    auto const cases{f.integer<uint8_t>()};
    for(unsigned int c{};c<cases;++c){
      std::vector<uint32_t> devices(f.integer<uint8_t>());for(auto &id:devices)id=f.integer<uint32_t>();hmp_player player{song,devices};
      for(auto const route:player.routing()){if(route.value_or(255)!=f.integer<uint8_t>())throw std::runtime_error{name+" device routing"};++routes;}
      if(!f.integer<uint8_t>())continue;
      auto const ticks{f.integer<uint32_t>()};bool const active{f.integer<uint8_t>()!=0};auto const packet_count{f.integer<uint32_t>()};
      struct event{uint32_t tick; midi_packet packet;};std::vector<event> expected;
      for(uint32_t p{};p<packet_count;++p){event e;e.tick=f.integer<uint32_t>();e.packet.device=f.integer<uint32_t>();auto const offset{f.integer<int32_t>()};if(offset>=0)e.packet.source_offset=static_cast<uint32_t>(offset);auto const wire{f.take(f.integer<uint16_t>())};e.packet.wire.assign(wire.begin(),wire.end());expected.push_back(std::move(e));}
      size_t cursor{};
      for(uint32_t tick{1};tick<=ticks;++tick)for(auto const &actual:player.tick()){
        if(cursor>=expected.size())throw std::runtime_error{std::format("{} case {} tick {}: excess packet",name,c,tick)};
        auto const &e{expected[cursor]};
        if(tick!=e.tick || actual.device!=e.packet.device || actual.wire!=e.packet.wire || actual.source_offset!=e.packet.source_offset)
          throw std::runtime_error{std::format("{} case {} packet {}: tick {} expected {}, source {} expected {}",name,c,cursor,tick,e.tick,actual.source_offset.value_or(0),e.packet.source_offset.value_or(0))};
        ++cursor;++packets;
      }
      if(cursor!=expected.size() || player.playing()!=active)throw std::runtime_error{name+" final playback state"};
    }
  }
  f.require_end();std::cout<<count<<" songs: "<<deltas<<" original deltas, "<<routes<<" routes and "<<packets<<" timed scheduler packets match\n";return 0;
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return 1;}
