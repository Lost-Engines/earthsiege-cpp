#include "render/hangar_artwork.h"
#include "resources/archive.h"
#include <iostream>
#include <stdexcept>
using namespace earthsiege;
int main(int argc,char **argv)try{
 if(argc!=3)return 1;
 resource_store r{argv[1]};hangar_artwork art{r};auto data{read_file(argv[2])};binary_reader f{data};auto magic{f.take(8)};
 if(std::string(magic.begin(),magic.end())!="ESBAYDM1")throw std::runtime_error{"fixture version"};
 auto count{f.integer<uint32_t>()};
 for(unsigned i{};i<count;++i){
  player_herc h;h.chassis=f.integer<int16_t>();for(auto &v:h.condition.armour)v=f.integer<int16_t>();
  framebuffer frame;frame.clear(37);art.damage(frame,h);
  auto expected{f.integer<uint32_t>()},back{f.integer<uint32_t>()};
  if(crc32(frame.pixels())!=expected||expected!=back){
   std::cerr<<"damage art "<<i<<" "<<crc32(frame.pixels())<<" expected "<<expected<<'/'<<back<<'\n';throw std::runtime_error{"original damage overlay mismatch"};
  }
 }
 f.require_end();std::cout<<count<<" original body damage overlay rasters agree\n";return 0;
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return 1;}
