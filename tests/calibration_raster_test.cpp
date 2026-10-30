#include "render/calibration_artwork.h"
#include <iostream>
#include <fstream>
using namespace earthsiege;
int main(int argc,char **argv)try{
 if(argc!=3&&argc!=4)return 1;
 resource_store resources{argv[1]};calibration_artwork artwork{resources,argc==4};
 auto raw{read_file(argv[2])};binary_reader f{raw};auto magic{f.take(8)};
 if(std::string(magic.begin(),magic.end())!="ESJCALR1")f.fail("fixture version");
 auto count{f.integer<uint32_t>()};
 for(unsigned i{};i<count;++i){
  auto choice{f.integer<uint8_t>()};auto message{f.integer<int8_t>()};std::array<uint8_t,3> states;for(auto &v:states)v=f.integer<uint8_t>();
  auto crc{f.integer<uint32_t>()};framebuffer frame;frame.clear(37);artwork.draw(frame,choice,message,states);
  if(crc32(frame.pixels())!=crc){std::ofstream out{"build/calibration-actual.raw",std::ios::binary};auto const &p{frame.pixels()};out.write(reinterpret_cast<char const *>(p.data()),p.size());f.fail("original calibration pixels "+std::to_string(i));}
 }
 f.require_end();std::cout<<count<<" original calibration rasters agree\n";return 0;
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return 1;}
