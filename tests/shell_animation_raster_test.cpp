#include "render/shell_animation_artwork.h"
#include "resources/archive.h"
#include <iostream>
#include <stdexcept>
using namespace earthsiege;
int main(int argc,char **argv)try{
 if(argc!=3)return 1;
 resource_store r{argv[1]};auto data{read_file(argv[2])};binary_reader f{data};auto magic{f.take(8)};
 if(std::string(magic.begin(),magic.end())!="ESSHPIX1")throw std::runtime_error{"fixture version"};
 auto programs{f.integer<uint32_t>()};unsigned total{};
 for(unsigned p{};p<programs;++p){
  auto name{f.cstring()};auto count{f.integer<uint32_t>()};
  shell_animation player{read_presentation(r.read("shell0","res32/"+name+".ttm"))};random_state random{17};
  shell_animation_artwork painter{r};framebuffer visible,background;background.clear(37);
  for(unsigned step{};step<count;++step){
   auto elapsed{f.integer<int16_t>()},page{f.integer<int16_t>()};if(page>=0)player.select_page(static_cast<uint16_t>(page));
   player.advance(elapsed,random);visible=background;painter.apply(player.commands(),visible,background);
   auto expected{f.integer<uint32_t>()},expected_back{f.integer<uint32_t>()};
   if(crc32(visible.pixels())!=expected||crc32(background.pixels())!=expected_back){
    std::cerr<<name<<" step "<<step<<" page "<<player.state().page<<" raster "<<crc32(visible.pixels())<<'/'<<expected<<" background "<<crc32(background.pixels())<<'/'<<expected_back<<'\n';
    throw std::runtime_error{"original shell animation raster mismatch"};
   }
   ++total;
  }
 }
 f.require_end();std::cout<<programs<<" shell animations, "<<total<<" visible and background rasters agree\n";return 0;
}catch(std::exception const &e){std::cerr<<e.what()<<'\n';return 1;}
