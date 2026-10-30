#include "render/mission_preferences_artwork.h"
#include <iostream>
#include <fstream>
using namespace earthsiege;
int main(int argc,char **argv)try{
 if(argc!=3)return 1;
 resource_store r{argv[1]};mission_preferences_artwork view{r};
 auto data{read_file(argv[2])};binary_reader f{data};auto magic{f.take(8)};
 if(std::string(magic.begin(),magic.end())!="ESPRFR01")f.fail("fixture version");
 auto count{f.integer<uint32_t>()};
 for(unsigned i{};i<count;++i){
  std::array<uint8_t,7> values;std::array<uint8_t,9> faces;
  for(auto &v:values)v=f.integer<uint8_t>();
  for(auto &v:faces)v=f.integer<uint8_t>();
  auto expected{f.integer<uint32_t>()};
  framebuffer image;image.clear(37);view.draw(image,values,faces);
  if(crc32(image.pixels())!=expected){
   std::ofstream out{"/tmp/earthsiege-preferences-native.raw",std::ios::binary};out.write(reinterpret_cast<char const *>(image.pixels().data()),64000);
   std::cerr<<"case "<<i<<" raster mismatch\n";return 1;
  }
 }
 f.require_end();std::cout<<count<<" complete original in-flight preferences rasters agree\n";return 0;
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return 1;}
