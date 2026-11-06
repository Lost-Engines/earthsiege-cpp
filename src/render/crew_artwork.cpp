#include "crew_artwork.h"
#include "resources/packed_strings.h"
#include "resources/resource_store.h"
#include <numeric>
namespace earthsiege {
crew_artwork::crew_artwork(resource_store const &r)
 :portraits{read_bitmaps(r.read("shell0","res32/pilots.dba"))},
 mechs{read_bitmaps(r.read("shell0","res32/mechs.dba"))},
 insignia{read_bitmaps(r.read("shell0","res32/insignia.dba")).front()},
 mesh{read_bitmaps(r.read("shell0","res32/mesh.dba")).front()},
 faces{read_font(r.read("shell0","dfn/nm_white.dfn")),read_font(r.read("shell0","dfn/nm_ylw.dfn")),read_font(r.read("shell0","dfn/big_red.dfn"))},
 text{r.read("shell0","eng/watext.bin"),r.edition()}{}
void crew_artwork::draw(framebuffer &f,dialog const &d,hangar const &h,career_state const &c,
 crew_assignment const &edit,std::string const &player_name)const{
 // GO 0x17a19/0x17b4a/0x17d9f and the value-image painter 0x1ed21.
 for(size_t i{};i<4;++i){
  auto const &member{c.crew[i]};auto const view{d.find(static_cast<uint16_t>(1611+i))};
  auto const body{d.find(static_cast<uint16_t>(1615+i))};auto const stats{d.find(static_cast<uint16_t>(1619+i))};
  if(!view||!body||!stats)continue;
  auto const &head{i==0?insignia:portraits.at(static_cast<size_t>(member.name))};
  f.image(head,view->bounds.left+1,view->bounds.top+1);
  if(i && c.current && !member.active)f.image(mesh,view->bounds.left,view->bounds.top,true);
  auto const chassis{h.find(member.hangar_slot)};
  {
   screen_mask mask;for(int y{body->bounds.top+1};y<body->bounds.bottom;++y)mask.rows.at(static_cast<size_t>(y)).push_back({body->bounds.left+1,body->bounds.right-1});
   scoped_framebuffer_clip clip{f,&mask};f.fill(body->bounds,31);
   if(chassis)f.image(mechs.at(human_herc_index(chassis->chassis)),body->bounds.left+1,body->bounds.top+1);
  }
  if(static_cast<int>(i)==edit.selected_pilot()||static_cast<int>(i)==edit.displaced_pilot()){
   auto const &b{body->bounds};uint8_t const colour{static_cast<uint8_t>(static_cast<int>(i)==edit.selected_pilot()?4:10)};
   f.line(b.left+1,b.top+1,b.right-1,b.top+1,colour);f.line(b.left+1,b.top+1,b.left+1,b.bottom-1,colour);
   f.line(b.right-1,b.top+1,b.right-1,b.bottom-1,colour);f.line(b.left+1,b.bottom-1,b.right-1,b.bottom-1,colour);
  }
  if(member.hangar_slot>=0)f.text(faces[2],std::to_string(member.hangar_slot+1),body->bounds.left+3,body->bounds.top+8);
  auto const &face{faces[i==static_cast<size_t>(c.crew_cursor)?1:0]};
  int const x{stats->bounds.left+2},y{stats->bounds.top+7};
  f.fill(stats->bounds,31);
  screen_mask stats_mask;for(int row{stats->bounds.top};row<=stats->bounds.bottom;++row)stats_mask.rows.at(static_cast<size_t>(row)).push_back({stats->bounds.left,stats->bounds.right});
  scoped_framebuffer_clip stats_clip{f,&stats_mask};
  f.text(face,i==static_cast<size_t>(c.selected_crew)?player_name:text.at(static_cast<size_t>(86+member.name*3+member.variant)),x,y);
  f.text(face,text.at(static_cast<size_t>(161+member.rank)),x,y+8);
  f.text(face,(chassis?text.chassis(chassis->chassis):text.at(166)),x,y+16);
  std::string condition{"---"};
  if(chassis){
   int total{std::accumulate(chassis->condition.armour.begin(),chassis->condition.armour.end(),0)};
   total=std::accumulate(chassis->condition.internal.begin(),chassis->condition.internal.begin()+9,total);
   total=std::accumulate(chassis->condition.equipment.begin(),chassis->condition.equipment.begin()+static_cast<ptrdiff_t>(chassis->weapons.size()),total);
   condition=std::to_string(total/static_cast<int>(22+chassis->weapons.size()))+"%";
  }
  f.text(face,condition,x,y+24);
 }
}
}
