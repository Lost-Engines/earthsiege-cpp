#include "render/wingmate_portrait.h"
#include <iostream>
int main(int argc,char **argv)try{
  using namespace earthsiege;if(argc!=3)return 1;resource_store resources{argv[1]};wingmate_portrait portraits{resources};auto const bytes{read_file(argv[2])};binary_reader file{bytes};auto const magic{file.take(8)};
  if(std::string(magic.begin(),magic.end())!="ESWPRT01")file.fail("fixture version");
  auto const count{file.integer<uint32_t>()};
  for(uint32_t i{};i<count;++i){
    auto const pilot{file.integer<int32_t>()},slot{file.integer<int32_t>()},mode{file.integer<int32_t>()},damage{file.integer<int32_t>()},status{file.integer<int32_t>()};
    rectangle const b{file.integer<int32_t>(),file.integer<int32_t>(),file.integer<int32_t>(),file.integer<int32_t>()};auto const crc{file.integer<uint32_t>()};framebuffer frame;frame.clear(123);
    if(mode)portraits.draw_static(frame,b,static_cast<size_t>(pilot),static_cast<size_t>(slot),static_cast<size_t>(status));
    else{std::array<int16_t,19> armour;armour.fill(static_cast<int16_t>(damage));portraits.draw_idle(frame,b,static_cast<size_t>(pilot),static_cast<size_t>(slot),armour,static_cast<int16_t>(status));}
    if(crc32(frame.pixels())!=crc){std::cerr<<"case "<<i<<" pilot "<<pilot<<" slot "<<slot<<" mode "<<mode<<" damage "<<damage<<" status "<<status<<'\n';file.fail("pilot labels/static pixels");}
  }
  file.require_end();std::cout<<count<<" original pilot idle/static portraits match\n";return 0;
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return 1;}
