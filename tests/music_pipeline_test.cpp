#include "audio/music_player.h"
#include "recording_pcm.h"
#include <iostream>
#include <format>
int main(int argc,char **argv)try{
  using namespace earthsiege;if(argc!=2)return 1;auto const raw{read_file(argv[1])};binary_reader r{raw};auto const magic{r.take(8)};
  if(std::string(magic.begin(),magic.end())!="ESMPIPE1")r.fail("fixture version");
  auto const melodic{r.take(r.integer<uint32_t>())},drums{r.take(r.integer<uint32_t>())},bank{r.take(r.integer<uint32_t>())};auto const count{r.integer<uint32_t>()};size_t writes{},events{};
  for(uint32_t c{};c<count;++c){auto const name{r.cstring()};bool const opl3{r.integer<uint8_t>()!=0};auto const song{r.take(r.integer<uint32_t>())};auto const ticks{r.integer<uint32_t>()};recording_backend backend;music_player player{opl3,melodic,drums,bank,backend};player.load(song);player.play();
    for(uint32_t step{};step<=ticks;++step){
      if(step){auto const finished{r.integer<uint8_t>()};for(unsigned int j{};j<finished;++j){auto const handle{r.integer<uint8_t>()};player.complete_sample(handle);backend.voices.erase(handle);}
        auto const op{r.integer<uint8_t>()};if(op==1)player.set_master(r.integer<uint8_t>());else if(op==2)player.stop();player.tick();}
      std::vector<opl_write> expected(r.integer<uint32_t>());for(auto &v:expected){v.reg=r.integer<uint16_t>();v.value=r.integer<uint8_t>();}
      auto const actual{player.take_registers()};if(actual!=expected){size_t j{};while(j<actual.size()&&j<expected.size()&&actual[j]==expected[j])++j;
        throw std::runtime_error{std::format("{} OPL{} step {} register {}: actual {:x}:{:x} expected {:x}:{:x}; counts {} / {}",name,opl3?3:2,step,j,j<actual.size()?actual[j].reg:0,j<actual.size()?actual[j].value:0,j<expected.size()?expected[j].reg:0,j<expected.size()?expected[j].value:0,actual.size(),expected.size())};}writes+=actual.size();
      std::vector<recording_backend::event> pcm(r.integer<uint32_t>());for(auto &e:pcm){e.action=r.integer<uint8_t>();e.handle=r.integer<uint8_t>();e.values.resize(e.action==0?6:e.action==1?0:1);for(auto &v:e.values)v=r.integer<uint32_t>();}
      if(pcm!=backend.events)throw std::runtime_error{std::format("{} OPL{} step {} digital events",name,opl3?3:2,step)};
      events+=pcm.size();backend.events.clear();
    }
  }r.require_end();std::cout<<count<<" original song/device cases: "<<writes<<" FM register writes and "<<events<<" PCM events match\n";return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}
