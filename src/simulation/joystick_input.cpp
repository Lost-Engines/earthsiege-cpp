#include "joystick_input.h"
#include "simulation/fixed_math.h"
#include <algorithm>
namespace earthsiege {
std::array<int32_t,16> joystick_calibration::values()const{
 std::array<int32_t,16> r{};for(size_t i{};i<4;++i){r[i]=minimum[i];r[i+4]=maximum[i];r[i+8]=centre_low[i];r[i+12]=centre_high[i];}return r;
}
joystick_calibration joystick_calibration::from_values(std::array<int32_t,16> const &v){
 joystick_calibration c;for(size_t i{};i<4;++i){c.minimum[i]=v[i];c.maximum[i]=v[i+4];c.centre_low[i]=v[i+8];c.centre_high[i]=v[i+12];}return c;
}
joystick_calibration read_joystick_calibration(bytes data){
 binary_reader r{data};std::array<int32_t,16> v;for(auto &x:v)x=r.integer<int32_t>();r.require_end();return joystick_calibration::from_values(v);
}
std::vector<uint8_t> write_joystick_calibration(joystick_calibration const &c){
 std::vector<uint8_t> out;for(auto v:c.values())for(unsigned shift{};shift<32;shift+=8)out.push_back(static_cast<uint8_t>(static_cast<uint32_t>(v)>>shift));return out;
}
joystick_sample sample_joystick(joystick_profile const &p,joystick_calibration const &c,std::array<int32_t,4> const &raw,uint8_t buttons){
 joystick_sample r;std::array<int32_t,4> v{};
 for(size_t i{};i<4;++i){
  if(p.axis_mask&(1<<i)){
   auto const x{std::min(raw[i],c.maximum[i])}; // Original lower clamp is overwritten.
   if(p.response==2)v[i]=raw[i]<c.centre_low[i]?-1:raw[i]>c.centre_high[i]?1:0;
   else{
    auto const a{x<c.centre_low[i]?c.centre_low[i]:c.centre_high[i]};
    auto const denominator{x<c.centre_low[i]?wrap_long(int64_t(a)-c.minimum[i]):x>c.centre_high[i]?wrap_long(int64_t(c.maximum[i])-a):0};
    if(denominator)v[i]=wrap_long((int64_t(x)-a)*128)/denominator;
   }
   auto const adjusted{v[i]>25?v[i]-25:v[i]<-25?v[i]+25:0};
   auto const scaled{wrap_long(int64_t(adjusted)*318)>>8};
   if(p.response==1){
    auto const word{wrap_word(scaled)};auto value{(int32_t(word)*word)>>6};if(scaled<0)value=-value;
    r.axes[i]=(value>-25&&value<25)?0:value;
   }else r.axes[i]=scaled;
  }
  r.buttons[i]=static_cast<uint8_t>((p.button_mask&buttons&(1<<i))!=0);
 }
 if(p.kind==joystick_kind::rudder)r.axes[2]=-r.axes[2];
 if(p.kind==joystick_kind::fcs){
  auto const x{v[3]};r.axes[3]=0;
  if(x<-16)r.hat[x<-112?0:x<-80?2:x<-48?1:3]=1;
 }
 if(p.kind==joystick_kind::flightstick||p.kind==joystick_kind::flightstick_pro){
  auto const x{wrap_word(v[3]*2+128)};auto y{(int32_t(x)*x)>>6};if(x<0)y=-y;r.axes[3]=y>-64&&y<64?0:y;
  if(p.kind==joystick_kind::flightstick_pro&&r.buttons[0]&&r.buttons[1]){
   r.hat[r.buttons[2]&&r.buttons[3]?0:r.buttons[2]?1:r.buttons[3]?2:3]=1;r.buttons={};
  }
 }
 return r;
}
std::array<joystick_profile,2> joystick_profiles(uint8_t model,bool pedals){
 std::array<joystick_profile,2> p{};for(auto &v:p){v.axis_mask=0;v.button_mask=0;}
 if(model){p[0].axis_mask=model<3?3:11;p[0].button_mask=model==3||model==5?15:3;}
 if(model==2){p[1].axis_mask=12;p[1].button_mask=12;}
 else if(pedals)p[1]={joystick_kind::rudder,4,0,1,0};
 if(model==3){p[0].kind=joystick_kind::fcs;p[0].hat=1;}
 if(model==4)p[0].kind=joystick_kind::flightstick;
 if(model==5){p[0].kind=joystick_kind::flightstick_pro;p[0].hat=1;}
 return p;
}
void combine_joystick(joystick_sample &to,joystick_sample const &from){
 for(size_t i{};i<4;++i){to.axes[i]|=from.axes[i];to.buttons[i]|=from.buttons[i];to.hat[i]|=from.hat[i];}
}
}
