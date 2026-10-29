#include "calibration_artwork.h"
namespace earthsiege {
calibration_artwork::calibration_artwork(resource_store const &r)
 :background{read_bitmaps(r.read("shell0","dba/cal_alrt.dba")).front()},
 corners{read_bitmaps(r.read("shell0","dba/corners.dba"))},styles{read_button_styles(r.read("zones","dat/styles.dat"))},
 title{read_font(r.read("shell0","dfn/title.dfn"))},metrics{read_font(r.read("shell0","dfn/gray.dfn"))},values{read_font(r.read("shell0","dfn/nm_green.dfn"))},
 faces{read_font(r.read("shell0","dfn/active.dfn")),read_font(r.read("shell0","dfn/pushed.dfn")),read_font(r.read("shell0","dfn/inactive.dfn"))},
 strings{read_interface_strings(r.read("shell0","str/cal_alrt.str"))}{
 auto data{r.read("shell0","bnd/cal_alrt.bnd")};binary_reader f{data};f.integer<uint32_t>();f.integer<int16_t>();
 width=f.integer<int16_t>();f.integer<int16_t>();title_padding=f.integer<int16_t>();title_top=f.integer<int16_t>();title_height=f.integer<int16_t>();title_offset=f.integer<int16_t>();
 if(f.integer<int16_t>()!=3)f.fail("calibration button count");
 for(auto &b:buttons)b.left=49+f.integer<int16_t>();
 for(auto &b:buttons)b.top=50+f.integer<int16_t>();
 for(auto &b:buttons)b.right=b.left+f.integer<int16_t>();
 for(auto &b:buttons)b.bottom=b.top+f.integer<int16_t>();
 for(auto &o:offsets)o=f.integer<int16_t>();
 value_field.left=49+f.integer<int16_t>();value_field.top=50+f.integer<int16_t>();
 value_field.right=value_field.left+f.integer<int16_t>();value_field.bottom=value_field.top+f.integer<int16_t>();value_x=f.integer<int16_t>();value_y=f.integer<int16_t>();
 prompt.left=49+f.integer<int16_t>();prompt.top=50+f.integer<int16_t>();prompt.right=prompt.left+f.integer<int16_t>();prompt.bottom=prompt.top+f.integer<int16_t>();f.require_end();
}
void calibration_artwork::draw(framebuffer &frame,unsigned choice,int message,std::span<uint8_t const,3> appearance)const{
 frame.image(background,49,50,true);
 auto const &caption{strings.at(0).at(0).text};int const w{title.text_width(caption)};
 rectangle const b{49+((width-w-title_padding)>>1),50+title_top,49+((width-w-title_padding)>>1)+w+title_padding,50+title_top+title_height};
 frame.line(b.left,b.top,b.right,b.top,16);frame.line(b.left,b.top,b.left,b.bottom,16);frame.line(b.right,b.top,b.right,b.bottom,27);frame.line(b.left,b.bottom,b.right,b.bottom,28);
 frame.image(corners.at(0),b.left,b.top,true);frame.image(corners.at(1),b.right-4,b.top,true);frame.image(corners.at(2),b.left,b.bottom-4,true);frame.image(corners.at(3),b.right-4,b.bottom-4,true);
 frame.text(title,caption,b.left+((b.right-b.left)>>1)-((w-1)>>1),b.top+((title_height+static_cast<int>(title.height))>>1)+title_offset);
 for(size_t i{};i<buttons.size();++i){
  auto const &rect{buttons[i]};auto const &face{faces.at(appearance[i])};auto const &text{strings.at(1).at(i).text};
  draw_dialog_button(frame,rect,styles.at(appearance[i]),corners);
  frame.text(face,text,rect.left+((rect.right-rect.left)>>1)-((face.text_width(text)-1)>>1),rect.top+((rect.bottom-rect.top+static_cast<int>(metrics.height))>>1)+offsets[i]);
 }
 frame.fill(value_field,31);
 frame.text(values,strings.at(2).at(choice).text,value_field.left+value_x,value_field.top+((value_field.bottom-value_field.top+static_cast<int>(values.height))>>1)+value_y);
 if(message>=0){auto const &text{strings.at(3).at(static_cast<size_t>(message)).text};
  frame.text(values,text,prompt.left+((prompt.right-prompt.left)>>1)-((values.text_width(text)-1)>>1),prompt.top+((prompt.bottom-prompt.top+static_cast<int>(values.height))>>1)+value_y);
 }
}
}
