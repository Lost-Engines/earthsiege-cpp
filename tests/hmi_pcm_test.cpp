#include "audio/hmi_pcm.h"
#include <iostream>
#include <format>
#include <map>
#include <vector>
#include "recording_pcm.h"
int main(int argc,char **argv)try{
  using namespace earthsiege;if(argc!=2)return 1;auto const raw{read_file(argv[1])};binary_reader r{raw};auto const magic{r.take(8)};
  if(std::string(magic.begin(),magic.end())!="ESPCMM01")r.fail("fixture version");
  auto const variants{r.integer<uint32_t>()};size_t commands{},events{};
  for(uint32_t variant{};variant<variants;++variant){recording_backend backend;hmi_pcm driver{r.take(r.integer<uint32_t>()),backend};auto const count{r.integer<uint32_t>()};
    for(uint32_t i{};i<count;++i){backend.events.clear();auto const op{r.integer<uint8_t>()};if(op){auto const h{r.integer<uint8_t>()};driver.complete(h);backend.voices.erase(h);}else driver.midi(r.take(r.integer<uint8_t>()));++commands;
      std::vector<recording_backend::event> expected(r.integer<uint32_t>());for(auto &e:expected){e.action=r.integer<uint8_t>();e.handle=r.integer<uint8_t>();e.values.resize(e.action==0?6:e.action==1?0:1);for(auto &v:e.values)v=r.integer<uint32_t>();}
      if(expected!=backend.events)throw std::runtime_error{std::format("variant {} command {}: digital backend events differ ({} / {})",variant,i,backend.events.size(),expected.size())};
      events+=expected.size();
      for(auto const &slot:driver.voices())for(auto const v:slot)if(v!=r.integer<int32_t>())throw std::runtime_error{std::format("variant {} command {}: ring entry",variant,i)};
      for(auto const v:driver.state())if(v!=r.integer<uint32_t>())throw std::runtime_error{std::format("variant {} command {}: ring state",variant,i)};
    }
  }r.require_end();std::cout<<commands<<" original digital MIDI operations and "<<events<<" backend events match\n";return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}
