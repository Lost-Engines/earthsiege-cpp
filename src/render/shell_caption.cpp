#include "shell_caption.h"
#include <algorithm>
namespace earthsiege {
shell_caption::shell_caption(resource_store const &r,std::string text,unsigned region)
 :face{read_font(r.read("shell0","dfn/nm_green.dfn"))},
  panel{read_bitmaps(r.read("shell0","res32/textbox.dba")).front()},
  area{region==7000?rectangle{108,160,299,195}:rectangle{8,7,284,29}},
  banner{region!=7000}{
 step=static_cast<int>(face.height)-2;
 rows.resize(static_cast<size_t>(face.text_width(text)/(area.right-area.left)+2));
 rows_per_page=(area.bottom-area.top)/step;
 pages=static_cast<int>(rows.size())/rows_per_page+(static_cast<int>(rows.size())!=rows_per_page);
 std::vector<size_t> starts{0};size_t previous{},line{};
 for(size_t i{};i<text.size();++i){
  if(text[i]=='\n'){text[i]=0;previous=i;starts.push_back(i+1);++line;}
  else if(text[i]==' '){
   text[i]=0;auto part{std::string_view{text.c_str()+starts[line]}};
   if(face.text_width(part)>area.right-area.left){
    if(text[previous]==' ')text[previous]=0;
    starts.push_back(previous+1);++line;
   }else previous=i;
   text[i]=' ';
  }
 }
 if(face.text_width(std::string_view{text.c_str()+starts[line]})>area.right-area.left){
  text[previous]=0;starts.push_back(previous+1);
 }
 // Bound malformed resources; the original writes beyond its estimated table.
 if(starts.size()>rows.size())throw std::runtime_error{"shell caption exceeds original line allocation"};
 for(size_t i{};i<starts.size();++i)rows[i]=std::string{text.c_str()+starts[i]};
}
void shell_caption::next(){if(selected<pages-1){++selected;banner=false;}}
void shell_caption::previous(){if(selected>0){--selected;banner=false;}}
void shell_caption::draw(framebuffer &target,uint8_t preference)const{
 if(banner){if(preference!=1)target.image(panel,3,4,true);}
 else target.fill(area,31);
 if(preference==1)return;
 int row{};
 for(int i{selected*rows_per_page};i<(selected+1)*rows_per_page&&i<static_cast<int>(rows.size());++i)
  if(rows[static_cast<size_t>(i)])target.text(face,*rows[static_cast<size_t>(i)],area.left,area.top+(++row)*step);
}
}
