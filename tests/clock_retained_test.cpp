#include "render/clock_instrument.h"
#include <iostream>
#include <fstream>
#include <memory>
using namespace earthsiege;
int main(int argc,char **argv)try{
 if(argc!=3)return 1;
 resource_store resources{argv[1]};auto raw{read_file(argv[2])};binary_reader file{raw};auto magic{file.take(8)};
 if(std::string(magic.begin(),magic.end())!="ESCLKPG1")file.fail("fixture version");
 std::string chassis;std::unique_ptr<clock_instrument> clock;rectangle bounds;
 auto count{file.integer<uint32_t>()};
 for(unsigned i{};i<count;++i){
  auto name{file.cstring()};
  if(name!=chassis){
   chassis=name;auto layout{read_cockpit_layout(resources.read("simvol0","gau/"+name+".GAU"))};
   bounds=layout.bounds(0x654);clock=std::make_unique<clock_instrument>(resources,layout);
  }
  auto now{file.integer<uint32_t>()};auto source{file.integer<uint8_t>()},target{file.integer<uint8_t>()};
  bool full{file.integer<uint8_t>()!=0},hidden{file.integer<uint8_t>()!=0},preparing{file.integer<uint8_t>()!=0};
  auto label{file.take(5)};bool colon{file.integer<uint8_t>()!=0};auto deadline{file.integer<uint32_t>()},expected{file.integer<uint32_t>()};
  bool visible{clock->visible_at(0,0,hidden)};
  if(full)clock->refresh_retained(target,visible);else clock->update_retained(now,source,target,visible,preparing);
  if(clock->text()!=std::string(label.begin(),label.end())||clock->colon_visible()!=colon||clock->next_update()!=deadline)file.fail("clock state "+std::to_string(i));
  std::vector<uint8_t> crop;
  for(unsigned page{};page<2;++page){
   framebuffer frame;frame.clear(63);clock->draw_retained(frame,page);
   for(int y{bounds.top};y<=bounds.bottom;++y)for(int x{bounds.left};x<=bounds.right;++x)crop.push_back(frame.pixels()[y*320+x]);
  }
  if(crc32(crop)!=expected){
   std::ofstream debug{"/tmp/earthsiege-clock-pages.raw",std::ios::binary};debug.write(reinterpret_cast<char const *>(crop.data()),crop.size());debug.close();
   file.fail("retained clock "+name+" step "+std::to_string(i));
  }
 }
 file.require_end();std::cout<<count<<" original retained clock-page rasters match\n";
 return 0;
}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}
