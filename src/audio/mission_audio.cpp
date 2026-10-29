#include "mission_audio.h"
#include "resources/interface_strings.h"
#include <algorithm>
#include <utility>

namespace earthsiege {
mission_audio::mission_audio(resource_store const &source):resources{source}{
  auto const groups{read_interface_strings(source.read("simvol0","str/SOUNDS.STR"))};size_t id{};
  for(auto const &group:groups)for(auto const &item:group){
    if(item.text.empty())return;
    if(item.attributes.size()!=4 || id>=47)throw std::runtime_error{"invalid original simulation sound catalogue"};
    if(normalise_path(item.text).ends_with(".sfx") || normalise_path(item.text).ends_with(".raw")){
      bank.bind(id,source.read("simsound",item.text));bank.set_loops(id,-1,-1,item.attributes[0]);bank.set_volume(id,static_cast<int8_t>(item.attributes[1]));
    }
    ++id;
  }
}
model_transform mission_audio::listener()const{model_transform result;result.translation=sources.listener();return result;}
void mission_audio::update_gain(size_t const id){
  auto const &s{bank.sound(id)};auto const gain{spatial_audio_gain(positions[id],listener(),s.volume,100000,10,s.pan,positioned[id])};
  bank.set_positioned(id,positioned[id],gain.pan);if(gain.gain)bank.set_spatial_volume(id,static_cast<int8_t>(*gain.gain));
}
void mission_audio::position(size_t const id,vec3<int32_t> const &where){positions.at(id)=where;positioned[id]=true;update_gain(id);}
void mission_audio::dispatch(sound_request const &request,vec3<int32_t> const &origin){
  if(auto const *sample{std::get_if<sample_sound_request>(&request)}){
    switch(sample->action){
      case sound_action::play:play(sample->id);break;
      case sound_action::positioned:play_at(sample->id,sample->position);break;
      case sound_action::stop:stop(sample->id);break;
      case sound_action::continuous:if(playing(sample->id))submit_position(sample->id,sample->position,origin);else play_at(sample->id,sample->position);break;
    }
  }else if(auto const *servo{std::get_if<servo_sound_request>(&request)}){
    if(auto const action{advance_servo_sound(servos.at(servo->axis),servo->control,servo->angle,playing(33))}){
      if(*action==sound_action::stop)stop(33);else play(33);
    }
  }else if(auto const *target{std::get_if<target_sound_request>(&request)}){
    if(auto const id{advance_target_sound(target_tone,target->locked,target->changed,static_cast<uint32_t>(ticks/2))})play(*id);
  }
}
void mission_audio::engine(bool const external_view){
  if(auto const action{engine_sound_gate(external_view,playing(45),playing(19))}){
    if(*action==sound_action::stop)stop(45);else play(45);
  }
}

bool mission_audio::play(size_t const id){return bank.play(id);}
bool mission_audio::play_at(size_t const id,vec3<int32_t> const &where){position(id,where);return play(id);}
bool mission_audio::speech(std::string_view const path){
  for(size_t slot{47};slot<57;++slot)if(!bank.playing(slot)){
    for(auto const volume:{"speech","simvoice"})if(auto const *entry{resources.archives().at(volume).find(path)}){
      bank.bind(slot,resources.archives().at(volume).read(*entry));return bank.play(slot);
    }
    return false;
  }
  return false;
}
void mission_audio::stop(size_t const id){bank.stop(id);}
bool mission_audio::playing(size_t const id)const{return bank.playing(id);}
void mission_audio::submit_position(size_t const id,vec3<int32_t> const &where,vec3<int32_t> const &origin){sources.submit(id,where,origin);}
void mission_audio::flush_positions(vec3<int32_t> const &origin){
  // Original queued positions are applied against the retained listener first.
  auto const pending{sources.distances()};auto const locations{sources.sources()};
  for(size_t id{};id<pending.size();++id)if(pending[id])position(id,locations[id]);
  bool const refresh{sources.remaining()==1};sources.flush(origin);
  if(refresh)for(size_t id{};id<positioned.size();++id)if(positioned[id])update_gain(id);
}
void mission_audio::advance(unsigned int const timer_ticks){
  for(unsigned int i{};i<timer_ticks;++i){
    auto const next{++ticks*11025/120};auto data{mixer.advance(static_cast<uint16_t>((next-sample_frames)*2))};sample_frames=next;output.insert(output.end(),data.begin(),data.end());
  }
}
std::vector<uint8_t> mission_audio::take_samples(){return std::exchange(output,{});}
pcm_sound_bank const &mission_audio::state()const{return bank;}
} // namespace earthsiege
