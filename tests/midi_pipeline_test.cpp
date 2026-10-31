#include "audio/music_player.h"
#include "recording_pcm.h"
#include <iostream>
#include <algorithm>
#include <format>
int main(int argc,char **argv)try{
  using namespace earthsiege;if(argc!=2)return 1;auto const raw{read_file(argv[1])};binary_reader r{raw};auto const magic{r.take(8)};
  if(std::string(magic.begin(),magic.end())!="ESMMPU01")r.fail("fixture version");
  auto const melodic{r.take(r.integer<uint32_t>())},drums{r.take(r.integer<uint32_t>())},bank{r.take(r.integer<uint32_t>())};auto const count{r.integer<uint32_t>()};size_t writes{},events{};
  for(uint32_t c{};c<count;++c){auto const name{r.cstring()};auto const device{r.integer<uint32_t>()};auto const song{r.take(r.integer<uint32_t>())};auto const ticks{r.integer<uint32_t>()};recording_backend backend;std::vector<uint8_t> wire;music_player player{device,melodic,drums,bank,backend,[&](std::span<uint8_t const> packet){wire.insert(wire.end(),packet.begin(),packet.end());}};player.load(song);player.play();
    for(uint32_t step{};step<=ticks;++step){
      if(step){auto const finished{r.integer<uint8_t>()};for(unsigned int j{};j<finished;++j){auto const handle{r.integer<uint8_t>()};player.complete_sample(handle);backend.voices.erase(handle);}
        auto const op{r.integer<uint8_t>()};if(op==1)player.set_master(r.integer<uint8_t>());else if(op==2)player.stop();player.tick();}
      auto const expected{r.take(r.integer<uint32_t>())};
      if(!std::ranges::equal(wire,expected))throw std::runtime_error{std::format("{} device {:x} step {}: MIDI bytes {} expected {}",name,device,step,wire.size(),expected.size())};
      writes+=wire.size();wire.clear();
      std::vector<recording_backend::event> pcm(r.integer<uint32_t>());for(auto &e:pcm){e.action=r.integer<uint8_t>();e.handle=r.integer<uint8_t>();e.values.resize(e.action==0?6:e.action==1?0:1);for(auto &v:e.values)v=r.integer<uint32_t>();}
      if(pcm!=backend.events)throw std::runtime_error{std::format("{} device {:x} step {} digital events",name,device,step)};
      events+=pcm.size();backend.events.clear();
    }
  }r.require_end();std::cout<<count<<" original song/device cases: "<<writes<<" MIDI bytes and "<<events<<" PCM events match\n";return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}
