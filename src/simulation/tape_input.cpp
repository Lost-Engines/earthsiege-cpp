#include "tape_input.h"
#include "fixed_math.h"
namespace earthsiege {
device_input_sample route_tape_frame(device_routing_options &o,tape_frame const &f,bool suppress,int16_t throttle){
 if(!(f.buttons&0x200))o.switch_held=false;
 else if(o.mode&&!o.switch_held){o.mode=static_cast<uint8_t>(3-o.mode);o.switch_held=true;}
 device_input_sample p;
 p.axes={f.primary_axes.x,f.primary_axes.y,f.secondary_axes.x,f.secondary_axes.y};
 for(unsigned i{};i<4;++i){p.buttons[i]=uint8_t((f.buttons>>i)&1);p.hat[i]=uint8_t((f.buttons>>(i+4))&1);}
 if(!o.external&&!o.missile){
  p.aiming_camera=o.mode==2;
  if(suppress){size_t const base{p.aiming_camera?2u:0u};p.axes[base]=p.axes[base+1]=0;}
  if(o.wcs)p.axes[1]=throttle;
 }
 if(o.backturn_reverse&&p.axes[1]>0)p.axes[0]=wrap_word(-p.axes[0]);
 return p;
}
}
