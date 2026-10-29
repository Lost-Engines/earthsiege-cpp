#include "shell_animation_artwork.h"
#include <algorithm>
#include <stdexcept>
namespace earthsiege {
void shell_animation_artwork::apply(std::vector<shell_animation_command> const &commands,framebuffer &visible,framebuffer &background){
 for(auto const &c:commands){
  auto const &a{c.arguments};
  switch(c.opcode){
   case 0xf02f:{
    auto path{normalise_path(c.resource)};
    if(path.ends_with(".bmp"))path.replace(path.size()-4,4,".dba");
    shapes.at(static_cast<size_t>(a.at(0)))=read_bitmaps(resources.read("shell0",directory+path));break;
   }
   case 0x80:
    if(a.at(0)<0)for(auto &bank:shapes)bank.clear();
    else shapes.at(static_cast<size_t>(a.at(0))).clear();
    break;
   case 0x4204:
    for(int y{std::max(0,a.at(1))};y<=std::min(199,a.at(3));++y)
     for(int x{std::max(0,a.at(0))};x<=std::min(319,a.at(2));++x)
      background.pixel(x,y,visible.pixels()[static_cast<size_t>(y*320+x)]);
    break;
   case 0xa504:case 0xa514:case 0xa524:case 0xa534:{
    auto const &bank{shapes.at(static_cast<size_t>(a.at(3)))};
    if(a.at(2)<0||static_cast<size_t>(a.at(2))>=bank.size())break;
    auto const &b{bank[static_cast<size_t>(a.at(2))]};auto const flip{(c.opcode>>4)&3};
    for(unsigned y{};y<b.height;++y)for(unsigned x{};x<b.width;++x){
     auto colour{b.pixels[(flip&1?b.height-1-y:y)*b.width+(flip&2?b.width-1-x:x)]};
     if(colour||(b.flags&0x10))visible.pixel(a.at(0)+static_cast<int>(x),a.at(1)+static_cast<int>(y),colour);
    }
    break;
   }
   default:throw std::runtime_error{"unsupported shell animation drawing operation"};
  }
 }
}
}
