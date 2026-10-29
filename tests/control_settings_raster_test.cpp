#include "render/control_settings_artwork.h"
#include <iostream>
#include <fstream>
using namespace earthsiege;
int main(int argc,char **argv)try{
 if(argc!=3)return 1;
 resource_store resources{argv[1]};control_settings_artwork artwork{resources};
 auto raw{read_file(argv[2])};binary_reader f{raw};auto magic{f.take(8)};
 if(std::string(magic.begin(),magic.end())!="ESCTLR01")f.fail("fixture version");
 auto count{f.integer<uint32_t>()};
 for(unsigned i{};i<count;++i){
  std::array<uint8_t,5> values;for(auto &v:values)v=f.integer<uint8_t>();
  std::array<uint8_t,7> states;for(auto &v:states)v=f.integer<uint8_t>();
  auto expected{f.integer<uint32_t>()};framebuffer frame;frame.clear(37);artwork.draw(frame,values,states);
  if(crc32(frame.pixels())!=expected){
   std::ofstream dump{"build/controls-actual.raw",std::ios::binary};auto const &p{frame.pixels()};dump.write(reinterpret_cast<char const *>(p.data()),p.size());
   f.fail("original controls raster "+std::to_string(i));
  }
 }
 f.require_end();std::cout<<count<<" original controls rasters agree\n";return 0;
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return 1;}
