#pragma once
#include "resources/career.h"
namespace earthsiege {
/// Original 1610 preview swaps, including restoring a displaced pilot on
/// subsequent cycling and the separate Accept/Cancel restore rules.
class crew_assignment {
  std::array<int16_t,4> original_slots{};
  int16_t original_cursor{},original_selected{};
  int16_t pilot{},previous_choice{-1},origin{-1},displaced{-1};
  void restore_displaced(hangar &,career_state &);
  void assign(hangar &,career_state &);
public:
  crew_assignment(hangar &,career_state &);
  void select(hangar &,career_state &,int16_t member);
  void cycle(hangar &,career_state &,int direction);
  void none(hangar &,career_state &);
  void accept(hangar &,career_state &)const;
  void cancel(hangar &,career_state &)const;
  int16_t selected_pilot()const{return pilot;}
  int16_t displaced_pilot()const{return displaced;}
  std::array<int16_t,5> state()const{return {static_cast<int16_t>(1615+pilot),original_selected,previous_choice,origin,displaced};}
};
}
