#include "construction_artwork.h"
#include "resources/packed_strings.h"
namespace earthsiege {
construction_artwork::construction_artwork(resource_store const &r)
 :blueprints{read_bitmaps(r.read("shell0","res32/blueherx.dba"))},
 faces{read_font(r.read("shell0","dfn/nm_green.dfn")),read_font(r.read("shell0","dfn/nm_white.dfn")),read_font(r.read("shell0","dfn/nm_red1.dfn"))},
 text{read_packed_strings(r.read("shell0","eng/watext.bin"))}{}
void construction_artwork::draw(framebuffer &frame,construction_menu const &menu,hangar const &h,career_state const &c,service_prices const &prices)const{
 // GO 0x2845c: fixed positions relative to the MENU2000 root at (181,2).
 auto const ch{static_cast<size_t>(menu.chassis())};auto const &p{prices.chassis.at(ch)};
 constexpr std::array<int,8> mass{18,27,36,45,50,54,63,77},speed{84,80,76,71,64,56,52,46},capacity{3,3,4,5,6,7,8,9};
 auto write=[&](size_t face,std::string const &value,int x,int y){frame.text(faces[face],value,181+x,2+y);};
 frame.fill({195,29,301,150},31);frame.image(blueprints.at(ch),196,30);
 std::array<std::string,4> values{text.at(27+ch),std::to_string(mass[ch])+" TONS",std::to_string(speed[ch])+" KPH",std::to_string(capacity[ch])+" WEAPONS"};
 for(size_t i{};i<4;++i){write(0,text.at(196+i),75,34+static_cast<int>(i)*18);write(1,values[i],79,43+static_cast<int>(i)*18);}
 bool const building{menu.building(h)};
 if(building){
  frame.fill({197,113,299,119},4);
  auto const &label{text.at(204)};write(2,label,17+(102-faces[2].text_width(label))/2,117);
  write(0,text.at(200),17,126);
  write(1,std::to_string((p.construction_missions-h.construction_remaining)*100/p.construction_missions),67,126);
 }
 write(0,text.at(building?203:201),17,135);write(0,text.at(202),17,144);
 auto tons=[&](int32_t value){auto s{std::to_string(value)};if(s.size()<3)s.insert(0,3-s.size(),' ');return s+" TONS";};
 write(1,tons(building?h.construction_value:p.purchase/1000),87,135);
 write(1,tons(static_cast<int32_t>(static_cast<uint32_t>(c.budget)/1000)),87,144);
}
}
