#include "radio_banner.h"
#include <stdexcept>

namespace earthsiege {
radio_banner::radio_banner(resource_store const &resources):face{read_font(resources.read("simvol0","dfn/CPRED.DFN"))}{}
std::string radio_banner::format(std::string_view const name,std::string_view const message)const{
  return std::string{name}+": "+std::string{message.substr(0,74)};
}
void radio_banner::draw_system(framebuffer &frame,std::string_view text,rectangle b,bool show)const{
  scoped_framebuffer_clip clip{frame,nullptr};frame.fill(b,63);
  if(show)frame.text(face,text,b.left+((b.right-b.left)>>1)-(face.text_width(text)>>1),b.bottom-1);
}
rectangle radio_banner::bounds(std::string_view const formatted)const{
  int const width{face.text_width(formatted)};int const left{150-(width>>1)};
  return {left,100,left+width+20,110};
}
void radio_banner::draw(framebuffer &frame,std::string_view const formatted,std::optional<size_t> const speaker)const{
  scoped_framebuffer_clip clip{frame,nullptr};
  if(speaker && *speaker>=3)throw std::out_of_range{"radio speaker colour"};
  uint8_t const fill{speaker?std::array<uint8_t,3>{56,59,49}[*speaker]:uint8_t{63}};
  uint8_t const edge{speaker?static_cast<uint8_t>(fill-1):uint8_t{53}};
  auto const b{bounds(formatted)};frame.fill(b,fill);
  frame.line(b.left,b.top,b.right,b.top,edge);frame.line(b.left,b.top,b.left,b.bottom,edge);
  frame.line(b.left,b.bottom,b.right,b.bottom,edge);frame.line(b.right,b.top,b.right,b.bottom,edge);
  frame.text(face,formatted,160-(face.text_width(formatted)>>1),b.bottom-1);
}
} // namespace earthsiege
