#include "cockpit_transition.h"
#include <utility>
namespace earthsiege {
void cockpit_transition::request(int action){
  if(value.pending!=-1)return;
  auto v{value.view};
  switch(action){
    case 0:if(v==0)value.pending=0;break;
    case 1:if(v==1)value.pending=1;break;
    case 2:if(v!=4)value.pending=2;break;
    case 3:if(v==4)value.pending=3;break;
    case 4:if(v==0)value.pending=4;else if(v==3)value.pending=6;break;
    case 5:if(v==0)value.pending=5;else if(v==2)value.pending=6;break;
    case 6:if(v==2||v==3)value.pending=6;break;
    default:break;
  }
}
void cockpit_transition::present(){
  auto &s{value};
  if(s.pending!=-1&&s.prepared){
    switch(s.pending){
      case 0:++s.view;break;
      case 1:--s.view;break;
      case 2:s.view=4;break;
      case 3:s.view=s.saved_view;break;
      case 4:s.view=2;break;
      case 5:s.view=3;break;
      case 6:s.view=0;break;
      default:break;
    }
    // The horizontal pan performs its own swap before the regular present.
    if(s.pending>=4)std::swap(s.draw_page,s.other_page);
    s.pending=-1;s.prepared=0;s.cooldown=2;
  }
  std::swap(s.draw_page,s.other_page);
  if(s.cooldown){--s.cooldown;return;}
  if(s.prepared||s.pending==-1)return;
  if((s.pending==4||(s.pending==6&&s.view==3))&&s.draw_page!=0)return;
  if((s.pending==5||(s.pending==6&&s.view==2))&&s.draw_page!=2)return;
  s.prepared=1;
  // 222c0 makes external mode active during preparation; the other requests
  // retain their old view until 22904 finishes the pan on a later present.
  if(s.pending==2){s.saved_view=s.view;s.view=4;}
}
void cockpit_transition::reset(int view){
  value.view=view;value.pending=-1;value.prepared=0;value.cooldown=0;
  if(view!=4)value.saved_view=view;
}
} // namespace earthsiege
