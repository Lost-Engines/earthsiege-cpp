#include "cockpit_transition.h"
#include <utility>
namespace earthsiege {
std::array<rectangle,3> cockpit_edge_bounds(int view){
  switch(view){
    case 1:return {{{0,0,319,3},{0,0,5,200},{314,0,319,200}}};
    case 2:return {{{-1,197,318,200},{-315,0,-310,200},{-1,0,4,200}}};
    case 3:return {{{1,197,320,200},{315,0,320,200},{629,0,634,200}}};
    default:return {{{0,197,319,200},{0,0,5,200},{314,0,319,200}}};
  }
}
int cockpit_edge_request(int view,size_t edge){
  if(edge==0)return view==0?0:view==1?1:-1;
  if(edge==1)return view==0?5:view==3?6:-1;
  if(edge==2)return view==0?4:view==2?6:-1;
  return -1;
}
int cockpit_edge_cursor(int view,int x,int y){
  auto bounds{cockpit_edge_bounds(view)};
  for(size_t i{};i<bounds.size();++i)if(bounds[i].contains(x,y)){
    auto const request{cockpit_edge_request(view,i)};
    if(request==0)return 4;
    if(request==1)return 3;
    if(request==4||(request==6&&view==3))return 5;
    if(request==5||(request==6&&view==2))return 6;
    return 0;
  }
  return 0;
}
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
