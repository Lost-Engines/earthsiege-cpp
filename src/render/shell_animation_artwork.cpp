#include "shell_animation_artwork.h"
#include "render/line_raster.h"
#include <algorithm>
#include <stdexcept>
namespace earthsiege {

namespace {
std::string converted(std::string_view name){
 auto path{normalise_path(name)};
 for(auto const &[from,to]:{std::pair{".bmp",".dba"},{".scr",".dbm"},{".pal",".dpl"},{".fnt",".dfn"}})
  if(path.ends_with(from))path.replace(path.size()-4,4,to);
 return path;
}
void refresh_rgb(palette &value){
 for(size_t i{};i<256;++i){auto const &c{value.dac_colours[i]};
  value.colours[i]=display_colour(c);
 }
}
palette faded(palette const &base,int start,int count,int destination,int percent){
 auto result{base};auto const &target{base.dac_colours.at(static_cast<size_t>(destination))};
 // GO 0x4b532 truncates range/percentage arguments to bytes and uses an
 // inclusive range. Authored count 256 therefore changes only one colour.
 int const factor{static_cast<int>(static_cast<int8_t>(percent))*256/100};
 for(unsigned i{static_cast<uint8_t>(start)};i<=static_cast<unsigned>(static_cast<uint8_t>(start))+static_cast<uint8_t>(count);++i){
  if(i>=256)throw std::runtime_error{"cinematic palette range exceeds original table"};
  uint32_t packed{};
  for(unsigned c{};c<3;++c){
   auto const delta{factor*(static_cast<int>(base.dac_colours[i][c])-target[c])};
   packed|=((static_cast<uint32_t>(delta)>>8)+target[c])<<(c*8);
  }
  for(unsigned c{};c<3;++c)result.dac_colours[i][c]=static_cast<uint8_t>(packed>>(c*8));
 }
 refresh_rgb(result);return result;
}
}
void shell_animation_artwork::submit(palette const &value){
 colours_=value;refresh_rgb(colours_);frames.push_back({presented,colours_});
}

void shell_animation_artwork::apply(std::vector<shell_animation_command> const &commands,framebuffer &visible,framebuffer &background){
 frames.clear();if(!initialized){presented=visible;initialized=true;}
 for(auto const &c:commands){
  auto const &a{c.arguments};
  switch(c.opcode){
   case 0xf02f:{
    auto path{normalise_path(c.resource)};
    if(path.ends_with(".bmp"))path.replace(path.size()-4,4,".dba");
    shapes.at(static_cast<size_t>(a.at(0)))=read_bitmaps(resources.read("shell0",directory+path));break;
   }

   case 0xf01f:{
    auto const bitmap{read_bitmaps(resources.read("shell0",directory+converted(c.resource))).front()};
    visible.image(bitmap,0,0,(bitmap.flags&0x10)==0);
    for(unsigned y{};y<=std::min(199u,bitmap.height);++y)
     for(unsigned x{};x<=std::min(319u,bitmap.width);++x)background.pixel(static_cast<int>(x),static_cast<int>(y),visible.pixels()[y*320+x]);
    break;
   }
   case 0x1061:palette_bank=static_cast<uint8_t>(a.at(0));if(palettes.at(palette_bank))palette_dirty=true;break;
   case 0xf05f:palettes.at(palette_bank)=read_palette(resources.read("shell0",directory+converted(c.resource)));palette_dirty=true;break;
   case 0x2002:pen=static_cast<uint8_t>(a.at(0));brush=static_cast<uint8_t>(a.at(1));break;
   case 0x4114:case 0x4124:{
    auto const &base{palettes.at(palette_bank).value()};
    int const step{6-a.at(3)};if(step<=0)throw std::runtime_error{"cinematic fade has nonpositive step"};
    auto fade{[&](int amount){submit(faded(base,a.at(0),a.at(1),a.at(2),amount));}};
    if(c.opcode==0x4124){
     fade(256);presented=background;
     for(int amount{step};amount<=100;amount+=step)fade(amount);
    }else{
     for(int amount{100};amount>=0;amount-=step)fade(amount);
     presented=visible;background=visible;
    }
    submit(base);break;
   }
   case 0xa104:{
    int const x{a.at(0)},y{a.at(1)},right{x+a.at(2)},bottom{y+a.at(3)};
    visible.fill({std::min(x,right),std::min(y,bottom),std::max(x,right),std::max(y,bottom)},brush);break;
   }
   case 0xa0a4:draw_model_line(visible,{a.at(0),a.at(1)},{a.at(2),a.at(3)},{0,0,319,199},pen);break;
   case 0x80:
    if(a.at(0)<0){for(auto &bank:shapes)bank.clear();for(auto &bank:palettes)bank.reset();palette_bank=0;pen=brush=10;}
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
 if(palette_dirty){submit(palettes.at(palette_bank).value());palette_dirty=false;}
 presented=visible;
}
}
