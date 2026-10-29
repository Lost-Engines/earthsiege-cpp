#include "joystick_calibration.h"
#include <algorithm>
#include <limits>
#include <stdexcept>
namespace earthsiege {
namespace {
constexpr std::array<std::array<bool,8>,6> available{{
 {{0,0,0,0,0,0,0,0}},{{1,0,0,0,0,0,0,0}},{{1,1,0,0,0,0,0,0}},
 {{0,0,1,1,0,0,0,0}},{{0,0,0,0,1,0,0,0}},{{0,0,0,0,0,1,0,0}}
}};
}
joystick_calibration_state::joystick_calibration_state(uint8_t m,bool p,std::array<bool,2> connected,std::array<joystick_calibration,2> &d)
 :model{m},pedals{p},present{connected},devices{d}{
 if(model>5||(!model&&!pedals))throw std::invalid_argument{"no calibration device enabled"};
 choice_=model==3?2:model==4?4:model==5?5:model?0:7;configure();
}
void joystick_calibration_state::configure(){
 slot=choice_==1||choice_==7?2:1;group=choice_==3?2:choice_==6?3:choice_==7?4:1;
 steps=choice_==3||choice_==6?2:3;mask=choice_==1?12:choice_==3||choice_==6?8:choice_==7?4:3;
}
void joystick_calibration_state::reset(){stage_=0;active_=false;cancelled_=false;message_=-1;}
std::optional<joystick_calibration_write> joystick_calibration_state::advance(){
 if(stage_==steps){
  auto const at{selected_slot()};auto &live{devices.at(at)};
  for(unsigned i{};i<4;++i)if(mask&(1<<i)){
   backup_.minimum[i]=live.minimum[i];backup_.maximum[i]=live.maximum[i];
   backup_.centre_low[i]=steps==2?live.maximum[i]:live.centre_low[i];backup_.centre_high[i]=steps==2?live.maximum[i]:live.centre_high[i];
  }
  live=backup_;reset();return joystick_calibration_write{at,live};
 }
 if(!stage_){
  if(present.at(selected_slot())){backup_=devices.at(selected_slot());message_=group*3;}
  else{message_=0;active_=false;}
 }else message_=group*3+stage_;
 ++stage_;return {};
}
void joystick_calibration_state::activate(unsigned button){
 if(active_)return;
 switch(button){
 case 0:do{choice_=(choice_+1)%8;}while(!available[model][static_cast<size_t>(choice_)]&&!(choice_==7&&pedals));configure();break;
 case 1:reset();active_=true;advance();break;
 case 2:done_=true;break;
 default:throw std::out_of_range{"calibration button"};
 }
}
std::optional<joystick_calibration_write> joystick_calibration_state::sample(std::array<int32_t,4> const &raw){
 if(!active_||!present.at(selected_slot()))return {};
 auto &c{devices.at(selected_slot())};
 if(stage_==1){
  c.minimum.fill(std::numeric_limits<int32_t>::max());c.maximum.fill(std::numeric_limits<int32_t>::min());
  c.centre_low.fill(std::numeric_limits<int32_t>::max());c.centre_high.fill(std::numeric_limits<int32_t>::min());
 }
 for(unsigned i{};i<4;++i)if(mask&(1<<i)){
  if(stage_==1)c.minimum[i]=std::min(c.minimum[i],raw[i]);
  else if(stage_==2)c.maximum[i]=std::max(c.maximum[i],raw[i]);
  else if(stage_==3){
   c.centre_low[i]=std::min(c.centre_low[i],raw[i]);if(int64_t(c.centre_low[i])-c.minimum[i]+1==0)++c.centre_low[i];
   c.centre_high[i]=std::max(c.centre_high[i],raw[i]);if(int64_t(c.maximum[i])-c.centre_high[i]+1==0)++c.centre_high[i];
  }
 }
 return advance();
}
void joystick_calibration_state::cancel(){if(active_){reset();cancelled_=true;}}
std::array<int32_t,12> joystick_calibration_state::state()const{return {stage_,0,0,active_,choice_,slot,0,0,cancelled_,group,steps,mask};}
}
