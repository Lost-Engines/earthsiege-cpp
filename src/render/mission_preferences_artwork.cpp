#include "mission_preferences_artwork.h"
namespace earthsiege {
namespace {
constexpr std::array<rectangle,9> buttons{{{21,143,81,153},{21,155,81,165},{21,167,81,177},{21,179,81,189},{158,142,220,152},{158,154,220,164},{158,166,220,176},{155,181,227,191},{228,181,300,191}}};
constexpr std::array<size_t,9> labels{0,1,2,3,6,7,8,4,5};
}
mission_preferences_artwork::mission_preferences_artwork(resource_store const &r)
 :background{read_bitmaps(r.read("simvol0","dba/prf_alrt.dba")).front()},
 corners{read_bitmaps(r.read("simvol0","dba/corners.dba"))},styles{read_button_styles(r.read("simvol0","dat/styles.dat"))},
 title{read_font(r.read("simvol0","dfn/title.dfn"))},metrics{read_font(r.read("simvol0","dfn/cpylw.dfn"))},values{read_font(r.read("simvol0","dfn/green6x8.dfn"))},
 faces{read_font(r.read("simvol0","dfn/active.dfn")),read_font(r.read("simvol0","dfn/pushed.dfn")),read_font(r.read("simvol0","dfn/inactive.dfn"))},
 strings{read_interface_strings(r.read("simvol0","str/prf_alrt.str"))}{}
rectangle mission_preferences_artwork::button(unsigned i)const{return buttons.at(i);}
void mission_preferences_artwork::draw(framebuffer &frame,std::span<uint8_t const,7> state,std::span<uint8_t const,9> appearance)const{
 frame.image(background,0,111,true);
 auto const &caption{strings.at(0).at(0).text};auto const w{title.text_width(caption)};
 rectangle const b{(319-w-20)>>1,115,((319-w-20)>>1)+w+20,132};
 frame.line(b.left,b.top,b.right,b.top,16);frame.line(b.left,b.top,b.left,b.bottom,16);frame.line(b.right,b.top,b.right,b.bottom,27);frame.line(b.left,b.bottom,b.right,b.bottom,28);
 frame.image(corners.at(0),b.left,b.top,true);frame.image(corners.at(1),b.right-4,b.top,true);frame.image(corners.at(2),b.left,b.bottom-4,true);frame.image(corners.at(3),b.right-4,b.bottom-4,true);
 frame.text(title,caption,b.left+((b.right-b.left)>>1)-((w-1)>>1),b.top+((17+int(title.height))>>1)-11);
 for(size_t i{};i<9;++i){
  auto const rect{buttons[i]};auto const &face{faces.at(appearance[i])};auto const &text{strings.at(1).at(labels[i]).text};
  draw_dialog_button(frame,rect,styles.at(appearance[i]),corners);
  frame.text(face,text,rect.left+((rect.right-rect.left)>>1)-((face.text_width(text)-1)>>1),rect.top+((10+int(metrics.height))>>1)-1);
 }
 for(size_t i{};i<4;++i){
  int const y{143+int(i)*12};frame.fill({85,y,145,y+10},63);
  frame.text(values,strings.at(i==2?3:2).at(state[i]).text,87,y+((10+int(values.height))>>1)-1);
 }
 for(size_t i{};i<3;++i){
  int const end{227+(int(state[i+4])*18247>>16)},y{143+int(i)*12};
  frame.fill({227,y,298,y+7},63);
  for(int x{227};x<=end;++x)frame.line(x,y,x,y+7,x%2?49:50);
 }
}
}
