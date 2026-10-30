#include "render/wingmate_portrait.h"
#include <iostream>
int main(int argc,char **argv)try{
 using namespace earthsiege;if(argc!=2)return 1;
 auto data{read_file(argv[1])};binary_reader file{data};auto magic{file.take(8)};
 if(std::string(magic.begin(),magic.end())!="ESBLNK01")file.fail("fixture version");
 auto count{file.integer<uint32_t>()};unsigned checked{};
 for(uint32_t i{};i<count;++i){
  auto mask{file.integer<uint8_t>()};std::array<rectangle,3> bounds;std::array<uint32_t,3> expected;
  for(size_t slot{};slot<3;++slot){
   bounds[slot]={file.integer<int32_t>(),file.integer<int32_t>(),file.integer<int32_t>(),file.integer<int32_t>()};
   expected[slot]=file.integer<uint32_t>();
  }
  framebuffer frame;frame.clear(86);wingmate_portrait::draw_empty(frame,bounds,mask);
  for(size_t slot{};slot<3;++slot)if(!(mask&(1u<<slot))){
   std::vector<uint8_t> pixels;auto const &b{bounds[slot]};
   for(int y{b.top};y<=b.bottom;++y)for(int x{b.left};x<=b.right;++x)pixels.push_back(frame.pixels()[static_cast<size_t>(y*320+x)]);
   if(crc32(pixels)!=expected[slot])throw std::runtime_error{"empty portrait mask "+std::to_string(mask)+" slot "+std::to_string(slot)};
   ++checked;
  }
 }
 file.require_end();std::cout<<checked<<" original empty portrait rasters agree across "<<count<<" crew masks\n";return 0;
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return 1;}
