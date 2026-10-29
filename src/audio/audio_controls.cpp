#include "audio_controls.h"
#include "simulation/fixed_math.h"
namespace earthsiege {
std::optional<sound_action> advance_servo_sound(servo_sound_state &s,int16_t const control,int16_t const angle,bool const playing){
  auto const magnitude{control<0 ? wrap_word(-static_cast<int32_t>(control)) : control};
  std::optional<sound_action> result;
  if(!s.active && magnitude>192 && s.previous!=angle){
    if(!playing){result=sound_action::play;s.active=true;}
  }else if(s.active && (magnitude==0 || s.previous==angle)){result=sound_action::stop;s.active=false;}
  s.previous=angle;return result;
}
std::optional<uint16_t> advance_target_sound(target_sound_state &s,bool const locked,bool const changed,uint32_t const clock){
  if(locked){s.previous_lock=true;if(!(clock&64))s.pulse=false;else if(!s.pulse){s.pulse=true;return 21;}}
  else if(s.previous_lock){s.previous_lock=false;return 22;}
  else if(changed)return 20;
  return {};
}
std::optional<sound_action> engine_sound_gate(bool const external_view,bool const engine_playing,bool const startup_playing){
  if(external_view && engine_playing)return sound_action::stop;
  if(!external_view && !engine_playing && !startup_playing)return sound_action::play;
  return {};
}
} // namespace earthsiege
