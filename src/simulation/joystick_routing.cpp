#include "joystick_routing.h"
#include "simulation/fixed_math.h"
namespace earthsiege {
device_input_sample route_devices(device_routing_options &o,joystick_sample k,joystick_sample d,int16_t throttle){
 if(d.buttons[1]){
  if(o.mode&&!o.switch_held){o.mode=static_cast<uint8_t>(3-o.mode);o.switch_held=true;}
 }else o.switch_held=false;
 if(o.external||o.missile){
  if(o.missile&&!o.missile_reverse)k.axes[1]=-k.axes[1];
  if(d.axes[0])k.axes[0]=d.axes[0];
  if(d.axes[1])k.axes[1]=d.axes[1];
  k.axes[2]=k.axes[3]=0;d.axes={};
 }else if(o.mode==1){
  if(k.axes[0]){k.axes[2]=k.axes[0];k.axes[0]=0;}
  if(k.axes[1]){k.axes[3]=-k.axes[1];k.axes[1]=0;}
 }
 if(o.tilt_reverse)k.axes[3]=-k.axes[3];
 auto const replace{[&](size_t to,size_t from){if(d.axes[from])k.axes[to]=d.axes[from];}};
 bool const wcs{o.wcs&&!o.missile};
 if(!o.model){if(o.pedals)replace(2,2);}
 else if(o.model==2){
  if(o.mode==2){replace(2,0);replace(3,1);replace(0,2);if(!wcs)replace(1,3);}
  else{replace(0,0);if(!wcs)replace(1,1);replace(2,2);replace(3,3);}
 }else{
  if(o.mode==2){replace(2,0);replace(3,1);if(o.pedals)replace(0,2);}
  else{replace(0,0);if(!wcs)replace(1,1);if(o.pedals)replace(2,2);}
 }
 if(o.model)for(size_t i{};i<4;++i)if((i<2||o.model!=1)&&d.buttons[i])k.buttons[i]=d.buttons[i];
 device_input_sample result;result.buttons=k.buttons;result.hat=d.hat;result.aiming_camera=!o.external&&!o.missile&&o.mode==2;
 for(size_t i{};i<4;++i)result.axes[i]=wrap_word(k.axes[i]);
 if(!o.external&&!o.missile&&wcs)result.axes[1]=throttle;
 if(o.backturn_reverse&&result.axes[1]>0)result.axes[0]=wrap_word(-result.axes[0]);
 return result;
}
}
