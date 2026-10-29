#include "audio/music_loops.h"
#include "resources/binary_reader.h"
#include <iostream>
int main(int argc,char **argv)try{
  using namespace earthsiege;if(argc!=2)return 1;auto const raw{read_file(argv[1])};binary_reader f{raw};auto const magic{f.take(8)};
  if(std::string(magic.begin(),magic.end())!="ESMLOOP1")f.fail("fixture version");
  auto const count{f.integer<uint32_t>()};
  for(uint32_t i{};i<count;++i){bool const playing{f.integer<uint8_t>()!=0};auto const branch{f.integer<uint8_t>()};music_loop_state state{f.integer<uint16_t>(),f.integer<uint16_t>()};auto const result{state.allow(branch,playing)};
    if(result!=(f.integer<uint8_t>()!=0)||state.configured!=f.integer<uint16_t>()||state.remaining!=f.integer<uint16_t>())throw std::runtime_error{"music loop decision"};}
  f.require_end();std::cout<<count<<" original music object loop decisions match\n";return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}
