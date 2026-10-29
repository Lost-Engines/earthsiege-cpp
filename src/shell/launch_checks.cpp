#include "launch_checks.h"
#include <algorithm>
namespace earthsiege {
namespace {
bool disabled(player_herc const &h){
 auto const &c{h.condition.internal};return c[0]<50||c[1]<50||c[5]<50;
}
player_herc const *selected(hangar const &h,career_state const &c){
 if(c.selected_crew<0||c.selected_crew>=4)return nullptr;
 return h.find(c.crew[static_cast<size_t>(c.selected_crew)].hangar_slot);
}
template<typename Predicate>
bool squad(hangar &h,career_state &c,Predicate predicate){
 auto count{std::ranges::count_if(c.crew,[](auto const &p){return p.active&&p.hangar_slot!=-1;})};
 bool found{};
 for(decltype(count) i{};i<count;++i){
  // The first iteration intentionally uses the retained cursor, even if that
  // pilot is inactive. GO advances only after testing this entry.
  if(c.crew_cursor>=0&&c.crew_cursor<4)
   if(auto p{h.find(c.crew[static_cast<size_t>(c.crew_cursor)].hangar_slot)})found=predicate(*p)||found;
  auto const previous{c.crew_cursor};
  do{c.crew_cursor=static_cast<int16_t>((c.crew_cursor+1)%4);}
  while(!c.crew[static_cast<size_t>(c.crew_cursor)].active||c.crew[static_cast<size_t>(c.crew_cursor)].hangar_slot==-1);
  if(c.crew_cursor!=previous)h.selected=c.crew[static_cast<size_t>(c.crew_cursor)].hangar_slot;
 }
 return found;
}
}
unsigned launch_warning(hangar &h,career_state &c){
 if(c.selected_crew<0||c.selected_crew>=4||c.crew[static_cast<size_t>(c.selected_crew)].hangar_slot==-1)return 3333;
 return squad(h,c,[](auto const &p){return std::ranges::all_of(p.weapons,[](int16_t w){return w==17;});})?3000:0;
}
unsigned next_mission_warning(hangar &h,career_state &c){
 if(auto p{selected(h,c)};p&&disabled(*p))return 7770;
 return squad(h,c,disabled)?7771:0;
}
bool construction_available(hangar const &h){
 return h.hercs.size()-(h.find(h.construction_slot)?1u:0u)<8;
}
}
