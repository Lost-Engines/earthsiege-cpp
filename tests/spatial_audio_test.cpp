#include "audio/spatial_audio.h"
#include "resources/binary_reader.h"
#include <iostream>

int main(int argc,char **argv)try{
  using namespace earthsiege;if(argc!=2)return 1;auto const data{read_file(argv[1])};binary_reader file{data};auto const magic{file.take(8)};if(std::string(magic.begin(),magic.end())!="ESSPAT01")file.fail("fixture version");
  auto const vector{[&]{return vec3<int32_t>{file.integer<int32_t>(),file.integer<int32_t>(),file.integer<int32_t>()};}};
  auto const count{file.integer<uint32_t>()};
  for(uint32_t i{};i<count;++i){
    auto const cutoff{file.integer<int32_t>()};auto const shift{file.integer<uint8_t>()};auto const angle{file.integer<uint16_t>()};auto const point{vector()},origin{vector()};
    auto const base{file.integer<uint8_t>()};bool const enabled{file.integer<uint8_t>()!=0};auto const pan{file.integer<uint16_t>()};auto const gain{file.integer<int32_t>()};auto const wanted_pan{file.integer<uint16_t>()};
    auto listener{rotation_transform({0,0,angle})};listener.translation=origin;auto const result{spatial_audio_gain(point,listener,base,cutoff,shift,pan,enabled)};
    if((result.gain?static_cast<int>(*result.gain):-1)!=gain || result.pan!=wanted_pan){std::cerr<<"case "<<i<<" pan "<<result.pan<<" expected "<<wanted_pan<<'\n';file.fail("original spatial gain/pan");}
  }
  auto const cases{file.integer<uint32_t>()};sound_sources sources{vector()};
  for(uint32_t i{};i<cases;++i){
    auto const origin{vector()};bool const enabled{file.integer<uint8_t>()!=0};
    for(size_t j{};j<6;++j){auto const id{file.integer<uint32_t>()};sources.submit(id,vector(),origin,enabled);}
    std::vector<positional_sound> pending;
    for(size_t id{};id<8;++id){auto const expected{file.integer<int32_t>()};if(sources.distances()[id]!=expected)file.fail("source coalescing distance");if(expected)pending.push_back({id,vec3<int32_t>{sources.sources()[id]}});}
    for(size_t id{};id<8;++id)if(sources.sources()[id]!=vector())file.fail("source coalescing position");
    auto const flushed{sources.flush(origin,enabled)};if(flushed.size()!=(enabled?pending.size():0))file.fail("source flush count");
    for(size_t j{};j<flushed.size();++j)if(flushed[j].id!=pending[j].id || flushed[j].position!=pending[j].position)file.fail("source flush order");
    if(sources.remaining()!=file.integer<uint32_t>() || sources.listener()!=vector())file.fail("listener refresh");
  }
  file.require_end();std::cout<<count<<" original gain/pan states and "<<cases*6<<" source submissions match\n";return 0;
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return 1;}
