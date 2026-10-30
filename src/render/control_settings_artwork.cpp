#include "control_settings_artwork.h"
namespace earthsiege {
control_settings_artwork::control_settings_artwork(resource_store const &r,bool flight)
 :background{read_bitmaps(r.read(flight?"simvol0":"shell0","dba/ctl_alrt.dba")).front()},
 corners{read_bitmaps(r.read(flight?"simvol0":"shell0","dba/corners.dba"))},styles{read_button_styles(r.read(flight?"simvol0":"zones","dat/styles.dat"))},
 title{read_font(r.read(flight?"simvol0":"shell0","dfn/title.dfn"))},metrics{read_font(r.read(flight?"simvol0":"shell0",flight?"dfn/cpylw.dfn":"dfn/gray.dfn"))},values{read_font(r.read(flight?"simvol0":"shell0",flight?"dfn/green6x8.dfn":"dfn/nm_green.dfn"))},
 faces{read_font(r.read(flight?"simvol0":"shell0","dfn/active.dfn")),read_font(r.read(flight?"simvol0":"shell0","dfn/pushed.dfn")),read_font(r.read(flight?"simvol0":"shell0","dfn/inactive.dfn"))},
 strings{read_interface_strings(r.read(flight?"simvol0":"shell0","str/ctl_alrt.str"))}{
 if(flight){origin_x=48;origin_y=45;field_background=63;}
 auto data{r.read(flight?"simvol0":"shell0","bnd/ctl_alrt.bnd")};binary_reader f{data};
 f.integer<uint32_t>();width=f.integer<int16_t>();f.integer<int16_t>();
 title_padding=f.integer<int16_t>();title_top=f.integer<int16_t>();title_height=f.integer<int16_t>();title_offset=f.integer<int16_t>();
 if(f.integer<int16_t>()!=7)f.fail("controls button count");
 for(auto &b:buttons)b.left=origin_x+f.integer<int16_t>();
 for(auto &b:buttons)b.top=origin_y+f.integer<int16_t>();
 for(auto &b:buttons)b.right=b.left+f.integer<int16_t>();
 for(auto &b:buttons)b.bottom=b.top+f.integer<int16_t>();
 for(auto &o:offsets)o=f.integer<int16_t>();
 if(f.integer<int16_t>()!=5)f.fail("controls value count");
 for(auto &b:fields)b.left=origin_x+f.integer<int16_t>();
 for(auto &b:fields)b.top=origin_y+f.integer<int16_t>();
 for(auto &b:fields)b.right=b.left+f.integer<int16_t>();
 for(auto &b:fields)b.bottom=b.top+f.integer<int16_t>();
 field_x=f.integer<int16_t>();field_y=f.integer<int16_t>();f.require_end();
}
void control_settings_artwork::draw(framebuffer &frame,std::span<uint8_t const,5> state,std::span<uint8_t const,7> appearance)const{
 frame.image(background,origin_x,origin_y,true);
 auto const &caption{strings.at(0).at(0).text};int const w{title.text_width(caption)};
 rectangle const b{origin_x+((width-w-title_padding)>>1),origin_y+title_top,origin_x+((width-w-title_padding)>>1)+w+title_padding,origin_y+title_top+title_height};
 frame.line(b.left,b.top,b.right,b.top,16);frame.line(b.left,b.top,b.left,b.bottom,16);frame.line(b.right,b.top,b.right,b.bottom,27);frame.line(b.left,b.bottom,b.right,b.bottom,28);
 frame.image(corners.at(0),b.left,b.top,true);frame.image(corners.at(1),b.right-4,b.top,true);frame.image(corners.at(2),b.left,b.bottom-4,true);frame.image(corners.at(3),b.right-4,b.bottom-4,true);
 frame.text(title,caption,b.left+((b.right-b.left)>>1)-((w-1)>>1),b.top+((title_height+static_cast<int>(title.height))>>1)+title_offset);
 for(size_t i{};i<buttons.size();++i){
  auto const &rect{buttons[i]};auto const &face{faces.at(appearance[i])};auto const &text{strings.at(1).at(i).text};
  draw_dialog_button(frame,rect,styles.at(appearance[i]),corners);
  frame.text(face,text,rect.left+((rect.right-rect.left)>>1)-((face.text_width(text)-1)>>1),rect.top+((rect.bottom-rect.top+static_cast<int>(metrics.height))>>1)+offsets[i]);
 }
 constexpr std::array<size_t,5> groups{4,5,2,2,3};
 for(size_t i{};i<fields.size();++i){
  auto const &rect{fields[i]};frame.fill(rect,field_background);
  frame.text(values,strings.at(groups[i]).at(state[i]).text,rect.left+field_x,rect.top+((rect.bottom-rect.top+static_cast<int>(values.height))>>1)+field_y);
 }
}
}
