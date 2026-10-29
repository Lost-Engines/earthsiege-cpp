#include "weapon_preview.h"
#include <algorithm>
namespace earthsiege {
namespace {
std::string name(int index){
 if(index<0||index>17)throw std::runtime_error{"invalid weapon preview"};
 auto suffix{std::to_string(index<14?index:index-14)};if(suffix.size()<2)suffix.insert(0,"0");
 return "res32/w1800"+std::string{index<14?"w":"r"}+suffix+".ttm";
}
}
weapon_preview::weapon_preview(resource_store const &r,int index,framebuffer const &base,random_state &random)
 :player{read_presentation(r.read("shell0",name(index)))},art{r},background{base},visible{base}{
 // GO 0x37996 preloads page zero; the control selects page one at 0x1f359.
 advance(0,random);player.select_page(1);advance(0,random);
}
void weapon_preview::advance(int16_t ticks,random_state &random){
 visible=background;player.advance(ticks,random);art.apply(player.commands(),visible,background);
}
void weapon_preview::draw(framebuffer &f,rectangle b)const{
 for(int y{std::max(0,b.top)};y<=std::min(199,b.bottom);++y)
  for(int x{std::max(0,b.left)};x<=std::min(319,b.right);++x)f.pixel(x,y,visible.pixels()[static_cast<size_t>(y*320+x)]);
}
}
