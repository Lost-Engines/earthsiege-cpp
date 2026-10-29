#include "full_damage_display.h"
#include <algorithm>
#include "resources/name_table.h"

namespace earthsiege {
namespace {
int percent(int damage){return (256-damage)*100>>8;}
size_t grade(int damage){auto const h{percent(damage)};return h>=90?0:h>=74?1:h>=51?2:h>=1?3:4;}
int colour(paper_doll_region const &r,int damage){
  auto const g{grade(damage)};
  if(r.colour_mode==damage_colour_mode::exact_colour)return g==3?48:g==4?62:r.base_colour+(r.base_colour==60?1:-1)*static_cast<int>(g);
  return std::array{54,57,92,51,62}[g];
}
rectangle translate(rectangle b,int x,int y){b.left+=x;b.right+=x;b.top+=y;b.bottom+=y;return b;}
void remap(framebuffer &target,paper_doll_region const &region,int value,int ox,int oy,bitmap const *source=nullptr){
  auto const b{translate(region.bounds,ox,oy)};
  for(int y{std::max(0,b.top)};y<=std::min(199,b.bottom);++y)for(int x{std::max(0,b.left)};x<=std::min(319,b.right);++x){
    int pixel{target.pixels()[static_cast<size_t>(y*320+x)]};
    if(source && region.colour_mode==damage_colour_mode::shaded_range){
      auto const sx{x-ox},sy{y-oy};if(sx<0 || sy<0 || sx>=static_cast<int>(source->width) || sy>=static_cast<int>(source->height))continue;
      pixel=source->pixels[static_cast<size_t>(sy)*source->width+static_cast<size_t>(sx)];
    }
    int replacement{pixel};bool write{};
    switch(region.colour_mode){
      case damage_colour_mode::shaded_range:write=pixel>=region.base_colour && pixel<=region.base_colour+2;replacement+=value-region.base_colour;break;
      case damage_colour_mode::exact_colour:write=pixel==region.base_colour;replacement=value;break;
      case damage_colour_mode::additive:write=pixel!=63;replacement+=value;break;
      case damage_colour_mode::solid_range:write=pixel>=region.base_colour && pixel<=region.base_colour+2;replacement=value;break;
    }
    if(write)target.pixel(x,y,static_cast<uint8_t>(replacement));
  }
}
void label(framebuffer &target,font const &face,std::string_view text,rectangle const &b,uint8_t background,bool centred){
  target.fill(b,background);int const x{centred?b.left+((b.right-b.left)>>1)-((face.text_width(text)-1)>>1):b.left};
  target.text(face,text,x,b.top+((b.bottom-b.top+static_cast<int>(face.height))>>1)-1);
}
}
full_damage_display::full_damage_display(resource_store const &r)
 :weapon_art{read_bitmaps(r.read("simvol0","dba/WEAPONS.DBA"))},weapon_sizes{read_weapon_icon_sizes(r.read("simvol0","pdg/WEAPONS.PDG"))},
 definitions{read_weapon_catalogue(r.read("simvol0","dat/WEAPONS.DAT"))},
 health_fonts{read_font(r.read("simvol0","dfn/CPGREEN.DFN")),read_font(r.read("simvol0","dfn/CPYLW.DFN")),read_font(r.read("simvol0","dfn/CPORANGE.DFN")),read_font(r.read("simvol0","dfn/CPRED.DFN")),read_font(r.read("simvol0","dfn/CPGREY.DFN"))},
 yellow{health_fonts[1]},red{health_fonts[3]},strings{read_interface_strings(r.read("simvol0","str/STRINGS0.STR"))}{
  for(auto const &name:read_resource_names(r.read("simvol0","nam/MECHS.NAM")))chassis.push_back({read_paper_doll(r.read("simvol0","pdg/"+name+".PDG")),read_bitmaps(r.read("simvol0","dba/"+name+".DBA")),read_weapon_mounts(r.read("simvol0","gl/"+name+".GL"))});
}
std::vector<weapon_diagram> full_damage_display::diagrams(full_damage_state const &s)const{
  std::vector<weapon_diagram> result;if(s.model<0)return result;auto const &art{chassis.at(static_cast<size_t>(s.model))};
  for(size_t i{};i<art.diagram.mounts.size();++i){
    auto const &mount{art.diagram.mounts[i]};weapon_diagram d;
    auto const it{std::ranges::find(art.mounts,static_cast<int8_t>(i),&weapon_mount::equipment_slot)};d.slot=static_cast<size_t>(it-art.mounts.begin());
    if(d.slot>=s.weapons.size() || s.weapons[d.slot].definition<0){result.push_back(d);continue;}
    int const icon{definitions.weapons.at(static_cast<size_t>(s.weapons[d.slot].definition)).damage_icon};
    if(icon<0){result.push_back(d);continue;}
    d.frame=icon*2+mount[2];auto const &size{weapon_sizes.at(static_cast<size_t>(d.frame))};d.x=mount[0];d.y=mount[1];
    switch(mount[3]&7){case 2:d.x-=size[0];break;case 4:d.x-=size[0]>>1;break;default:break;}
    switch(mount[3]&~7){case 16:d.y-=size[1];break;case 32:d.y-=size[1]>>1;break;default:break;}
    d.bounds={d.x,d.y,d.x+size[0],d.y+size[1]};d.flags=mount[4];
    if(d.flags==2)d.x-=static_cast<int>(weapon_art.at(static_cast<size_t>(d.frame)).width)-size[0];
    result.push_back(d);
  }return result;
}
void full_damage_display::draw(framebuffer &target,rectangle const &bounds,rectangle const &listing,full_damage_state const &s)const{
  scoped_framebuffer_clip clip{target,nullptr};target.fill(bounds,31);if(s.failed)return;
  auto const subject{s.subject==0?strings.at(14).at(0).text:s.subject==4?strings.at(15).at(0).text:"RED "+std::to_string(s.subject)};
  label(target,s.subject==0?yellow:red,subject,{bounds.left+20,bounds.bottom-2-static_cast<int>(yellow.height),bounds.left+60,bounds.bottom-2},std::array<uint8_t,5>{50,56,59,49,59}.at(static_cast<size_t>(s.subject)),true);
  if(s.model<0){auto const x{bounds.left+((bounds.right-bounds.left)>>1)},y{bounds.top+((bounds.bottom-bounds.top)>>1)};label(target,red,strings.at(16).at(s.no_information?1:0).text,{x-40,y,x+40,y+10},59,true);return;}
  auto const &art{chassis.at(static_cast<size_t>(s.model))};auto const view_index{s.mode==1?1U:0U};auto const &view{art.diagram.views.at(view_index)};auto const &image{art.frames.at(view_index)};
  int const x{bounds.left+view.bounds.left},y{bounds.top+view.bounds.top};target.image(image,x,y,true);
  auto const mounts{diagrams(s)};
  auto const draw_weapons{[&]{for(auto const &d:mounts)if(d.frame>=0){auto const &b{weapon_art.at(static_cast<size_t>(d.frame))};
    for(unsigned row{};row<b.height;++row)for(unsigned column{};column<b.width;++column){auto const value{b.pixels[row*b.width+column]};if(value)target.pixel(x+d.x+static_cast<int>(d.flags==2?b.width-1-column:column),y+d.y+static_cast<int>(row),value);}
  }}};
  if(s.mode==2){
    remap(target,{0,{0,0,view.bounds.right,view.bounds.bottom},54,damage_colour_mode::solid_range},50,x,y);
    for(auto r:view.regions)if(r.base_colour!=54){r.colour_mode=damage_colour_mode::solid_range;remap(target,r,50,x,y);}
    draw_weapons();for(auto const &d:mounts)remap(target,{0,d.bounds,54,damage_colour_mode::exact_colour},56,x,y);
  }else{draw_weapons();for(auto const &d:mounts)remap(target,{0,d.bounds,56,damage_colour_mode::exact_colour},50,x,y);}
  size_t const count{s.mode==2?mounts.size():view.regions.size()};
  for(size_t i{};i<count;++i){
    paper_doll_region region;int damage{},paint_damage{};std::string name;bool paint{true};
    if(s.mode==2){auto const &d{mounts[i]};region={static_cast<int32_t>(d.slot),d.bounds,54,damage_colour_mode::shaded_range};
      damage=i<s.weapons.size()?s.weapons[i].damage:0;paint_damage=damage;
      if(d.slot<s.weapons.size())name=s.weapons[d.slot].name;
    }else{region=view.regions[i];auto const section{static_cast<size_t>(region.section)};
      damage=s.mode==0?s.armour.at(section):s.internal.at(section);paint_damage=damage;
      name=strings.at(s.mode==0?12:13).at(section).text;
      if(s.mode==0){if(section==0 || section==2)paint_damage=(damage+s.armour.at(section+1))>>1;else if(section==1 || section==3)paint=false;}
    }
    if(paint)remap(target,region,colour(region,paint_damage),x,y,s.mode==2?nullptr:&image);
    if(name.empty() || static_cast<int>(i)<s.scroll || static_cast<int>(i)>=s.scroll+12)continue;
    auto const &face{health_fonts[grade(damage)]};int const top{listing.top+(static_cast<int>(i)-s.scroll)*static_cast<int>(health_fonts[0].height)};
    int const divider{listing.right-health_fonts[0].text_width("100")};
    label(target,face,name,{listing.left+12,top,divider,top+4},63,false);
    label(target,face,std::to_string(percent(damage)),{divider,top,listing.right,top+4},63,false);
  }
}
} // namespace earthsiege
