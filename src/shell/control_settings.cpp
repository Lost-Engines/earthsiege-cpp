#include "control_settings.h"
#include <stdexcept>
namespace earthsiege {
control_settings_state::control_settings_state(preferences &p):prefs{p},persisted_{p.current()}{
 auto const &v{p.current()};state_[0]=v[7];state_[1]=v[8];state_[2]=v[10];state_[3]=v[9];state_[4]=v[11];
 state_[5]=v[7]==2?2:0;state_[6]=v[8]==0?1:v[8];state_[7]=v[9];state_[8]=v[7];state_[9]=v[9];
}
void control_settings_state::apply(){
 if(state_[0]!=state_[8]||state_[3]!=state_[9]){
  if(prefs.current()[7]==2&&prefs.current()[9])prefs.set(9,0);
  state_[8]=state_[0];state_[9]=state_[3];
 }
 for(size_t i{7};i<=11;++i)persisted_[i]=prefs.current()[i];
}
control_setting_action control_settings_state::activate(unsigned button,bool reverse,bool joystick_button){
 auto cycle{[&](size_t index,unsigned count){auto v{prefs.current()[index]};v=static_cast<uint8_t>(reverse?(v? v-1:count-1):(v+1==count?0:v+1));prefs.set(index,v);return v;}};
 switch(button){
 case 0:
  state_[0]=cycle(7,6);if(state_[0]==0&&joystick_button){auto v{static_cast<uint8_t>((prefs.current()[7]+1)%6)};prefs.set(7,v);state_[0]=v;}
  if(!state_[0]){
   if(state_[1]){state_[6]=state_[1];state_[1]=0;prefs.set(8,0);}
  }else{
   if(!state_[1]){state_[1]=state_[6];prefs.set(8,state_[1]);}
   if(state_[0]==2){if(state_[3]){state_[7]=state_[3];state_[3]^=1;}}
   else if(state_[7])state_[3]=state_[7];
  }break;
 case 1:if(state_[0]){do{state_[1]=cycle(8,3);}while(!state_[1]);}break;
 case 2:prefs.toggle(10);state_[2]=prefs.current()[10];break;
 case 3:
  prefs.toggle(9);state_[3]=state_[7]=prefs.current()[9];
  if(state_[3]&&state_[0]==2){state_[5]=2;state_[0]=1;prefs.set(7,1);}
  else if(!state_[3]&&state_[5]==2){state_[5]=0;state_[0]=2;prefs.set(7,2);}
  break;
 case 4:prefs.toggle(11);state_[4]=prefs.current()[11];break;
 case 5:if(state_[3]||state_[0]){apply();return control_setting_action::calibrate;}break;
 case 6:apply();prefs.commit();return control_setting_action::done;
 default:throw std::out_of_range{"control settings button"};
 }
 return control_setting_action::none;
}
}
