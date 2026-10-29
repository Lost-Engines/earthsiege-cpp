#include "cockpit_impact.h"
#include <algorithm>
#include <utility>
#include "simulation/fixed_math.h"
namespace earthsiege {
void cockpit_impact::start_shake(cockpit_impact_view const &view,int amount){
 if(view.mode==4||view.transitioning)return;
 current.saved_origin=current.current_origin=view.origin;
 if(view.mode==2||view.mode==3)current.saved_origin[0]=current.current_origin[0]=0;
 if(view.mode==1){current.upper=view.origin[1];current.lower=view.origin[1]-amount;}
 else if(view.mode<=3){current.lower=view.origin[1];current.upper=view.origin[1]+amount;}
 current.displacement=0;current.shaking=true;
}
void cockpit_impact::move(unsigned amount,unsigned mode){
 if(!current.shaking||mode==4)return;
 int const below{wrap_word(current.current_origin[1]-current.lower)};
 int const above{wrap_word(current.upper-current.current_origin[1])};
 int const delta{above>below?std::min(above,static_cast<int>(amount)):-std::min(below,static_cast<int>(amount))};
 current.current_origin[1]=wrap_long(static_cast<int64_t>(current.current_origin[1])+delta);
 current.displacement=wrap_word(current.displacement+delta);
}
void cockpit_impact::restore(unsigned mode){
 if(!current.shaking||mode==4)return;
 current.displacement=0;current.shaking=false;
}
void cockpit_impact::change_view(unsigned previous_mode,cockpit_impact_view const &view){
 if(previous_mode==view.mode)return;
 bool const resume{current.shaking};
 if(resume)restore(previous_mode);
 // 222c0 restores the old page; 242f6 resumes with ten pixels after the
 // transition. Selecting the external view cancels that resumption.
 if(resume&&view.mode!=4)start_shake(view,10);
}
void cockpit_impact::hit(uint32_t now,cockpit_impact_view const &view,random_state &random){
 if(view.mode==4)return;
 current.expiry=now+60;
 if(current.next_flash)return;
 current.next_flash=now+static_cast<uint16_t>(random.next())%10;
 if(view.palette_available){
  current.saved_palette=current.alternate_palette=current.impact_palette;
  current.impact_palette=true;current.flash=true;
 }
 start_shake(view);
}
bool cockpit_impact::update(uint32_t now,cockpit_impact_view const &view,random_state &random){
 if(view.mode==4)current.expiry=0;
 if(current.expiry){
  if(now<current.expiry){
   if(now>current.next_flash){
    if(view.palette_available){std::swap(current.impact_palette,current.alternate_palette);current.flash=!current.flash;}
    current.next_flash=now+static_cast<uint16_t>(random.next())%10;
   }
   move(static_cast<uint16_t>(random.next())%5,view.mode);
  }else current.expiry=0;
  return true;
 }
 if(current.next_flash){
  if(view.palette_available){current.impact_palette=current.saved_palette;current.flash=false;}
  current.next_flash=0;restore(view.mode);return true;
 }
 return false;
}
}
