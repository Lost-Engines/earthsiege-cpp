#include "pilot_transmission.h"
#include <algorithm>
#include <utility>

namespace earthsiege {
pilot_transmission::pilot_transmission(uint8_t const message_id,std::vector<portrait_frame> timeline,uint32_t const ticks)
  :message{message_id},animation{std::move(timeline)}{data.phase=1;data.deadline=ticks+20;}
void pilot_transmission::begin(uint8_t const id,std::vector<portrait_frame> timeline,uint32_t const ticks){
  message=id;animation=portrait_animation{std::move(timeline)};started=false;data.phase=1;data.deadline=ticks+20;
}
pilot_transmission_state const &pilot_transmission::state()const{return data;}
int16_t pilot_transmission::mouth()const{return started?animation.frame():0;}
pilot_transmission_update pilot_transmission::advance(uint32_t const ticks,bool const shown,bool const dead,random_state &random,std::optional<uint32_t> const animation_ticks,std::function<bool(int16_t)> const &play_speech){
  pilot_transmission_update result;
  auto const speech_ticks{animation_ticks.value_or(ticks)};
  if(started)animation.advance(speech_ticks);
  auto const paint{[&](int kind,int value=0){result.calls.push_back({kind,value});}};
  auto const idle{[&]{if(data.previous!=0)paint(0);}};
  switch(data.phase){
    case 0:idle();break;
    case 1:
      if(data.deadline>ticks || data.permanent_static){paint(1);break;}
      data.phase=2;
      [[fallthrough]];
    case 2:{
      bool playback_ok{true};
      if(data.previous!=data.phase){result.ready=true;animation.start(speech_ticks);started=true;if(play_speech)playback_ok=play_speech(animation.frame());if(message==37)data.deadline=ticks+5;}
      auto const frame{playback_ok?animation.frame():int16_t{-1}};
      if(frame==-1){
        result.cancel=true;
        if(message==37){data.permanent_static=true;data.phase=1;}
        else{data.phase=3;data.deadline=ticks+20;paint(3,28);if(dead)data.permanent_static=true;}
        paint(1);
      }else if(message==37){
        if(shown){
          if(data.death_frame==0){paint(2,27);if(data.deadline<ticks)++data.death_frame;}
          else if(data.death_frame==1){paint(1);if(data.deadline<ticks)--data.death_frame;}
          if(data.death_frame!=data.previous_death_frame){
            auto delay{static_cast<uint32_t>(static_cast<int32_t>(random.bounded(20)))};
            data.deadline=ticks+std::max(delay,uint32_t{5});
          }
          data.previous_death_frame=data.death_frame;
        }
      }else paint(2,frame);
      break;
    }
    case 3:
      if(data.deadline<=ticks){data.phase=0;idle();}else paint(1);
      break;
    default:break;
  }
  data.previous=data.phase;return result;
}
} // namespace earthsiege
