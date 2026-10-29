#include "crew_assignment.h"
namespace earthsiege {
crew_assignment::crew_assignment(hangar &h,career_state &c)
 :original_cursor{c.crew_cursor},original_selected{h.selected}{
  for(size_t i{};i<4;++i)original_slots[i]=c.crew[i].hangar_slot;
  select(h,c,0);
}
void crew_assignment::select(hangar &h,career_state &c,int16_t member){
  if(member<0||member>=4)return;
  displaced=-1;pilot=member;c.crew_cursor=member;h.selected=c.crew[static_cast<size_t>(member)].hangar_slot;origin=h.selected;
}
void crew_assignment::restore_displaced(hangar &,career_state &c){
  if(displaced>=0&&pilot==c.crew_cursor)c.crew[static_cast<size_t>(displaced)].hangar_slot=c.crew[static_cast<size_t>(pilot)].hangar_slot;
}
void crew_assignment::assign(hangar &h,career_state &c){
  if(previous_choice==h.selected)h.selected=-1;
  previous_choice=h.selected;displaced=-1;
  if(h.selected>=0)for(int16_t i{};i<4;++i)if(c.crew[static_cast<size_t>(i)].hangar_slot==h.selected){displaced=i;break;}
  if(displaced>=0)c.crew[static_cast<size_t>(displaced)].hangar_slot=origin;
  c.crew.at(static_cast<size_t>(c.crew_cursor)).hangar_slot=h.selected;
}
void crew_assignment::cycle(hangar &h,career_state &c,int direction){
  restore_displaced(h,c);
  if(h.capacity<=0)return;
  for(int i{};i<h.capacity;++i){
    if(direction>0)h.selected=static_cast<int16_t>((h.selected+1)%h.capacity);
    else{if(h.selected<1)h.selected=h.capacity;--h.selected;}
    if(h.find(h.selected)&&h.selected!=h.construction_slot)break;
  }
  assign(h,c);
}
void crew_assignment::none(hangar &h,career_state &c){
  restore_displaced(h,c);h.selected=-1;origin=-1;assign(h,c);
}
void crew_assignment::accept(hangar &h,career_state &c)const{
  c.crew_cursor=c.selected_crew;h.selected=original_selected;
}
void crew_assignment::cancel(hangar &h,career_state &c)const{
  // GO restores the cursor before restoring the four assignments; retain
  // that ordering even when its transient selected-HERC value is surprising.
  h.selected=original_selected;c.crew_cursor=original_cursor;
  if(original_cursor>=0)h.selected=c.crew.at(static_cast<size_t>(original_cursor)).hangar_slot;
  for(size_t i{};i<4;++i)c.crew[i].hangar_slot=original_slots[i];
}
}
