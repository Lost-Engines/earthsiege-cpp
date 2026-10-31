#include "mission_audio.h"
#include "resources/interface_strings.h"
#include <algorithm>
#include <utility>

namespace earthsiege {
mission_audio::mission_audio(resource_store const &source,music_settings const &settings):resources{source},music_config{settings}{
  auto const groups{read_interface_strings(source.read("simvol0","str/SOUNDS.STR"))};size_t id{};
  for(auto const &group:groups)for(auto const &item:group){
    if(item.text.empty())return;
    if(item.attributes.size()!=4 || id>=47)throw std::runtime_error{"invalid original simulation sound catalogue"};
    if(id<songs.size())songs[id]=item.text;
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

bool mission_audio::preferences(bool const enable_music,bool const enable_effects,bool const enable_speech){
  // Original 4e294/4e2c8 stop registered songs/effects only on a falling edge.
  // Radio's 959e4 flag gates subsequent speech starts; it does not stop a line.
  if(music_enabled&&!enable_music&&music)music->stop();
  if(effects_enabled&&!enable_effects)for(size_t id{6};id<60;++id)bank.stop(id);
  bool const resumed{enable_music&&!music_enabled};
  music_enabled=enable_music;effects_enabled=enable_effects;speech_enabled=enable_speech;
  return resumed;
}
void mission_audio::update_music(music_contacts const &contacts,random_state &random){
  if(!music_enabled)return;
  if(!music)music=std::make_unique<music_audio>(resources,bank,music_config);
  if(music_state.track>=0)playback.playing[static_cast<size_t>(music_state.track)]=music->playing();
  for(auto const &event:advance_combat_music(music_state,playback,contacts.actors,contacts.flyers,static_cast<uint32_t>(ticks/2),true,true,random)){
    switch(event.action){
      case music_action::load:music->load(resources.read("simsound",songs.at(event.track)));if(observe)observe({audio_event_kind::music_load,event.track,songs.at(event.track)});break;
      case music_action::play:music->play();if(observe)observe({audio_event_kind::music_play,event.track,{}});break;
      case music_action::stop:music->stop();if(observe)observe({audio_event_kind::music_stop,event.track,{}});break;
      case music_action::volume:music->set_volume(event.value);if(observe)observe({audio_event_kind::music_volume,event.track,{}});break;
    }
  }
}

bool mission_audio::play(size_t const id){auto const accepted{effects_enabled&&bank.play(id)};if(observe)observe({audio_event_kind::play,id,{},accepted});return accepted;}
bool mission_audio::play_at(size_t const id,vec3<int32_t> const &where){position(id,where);return play(id);}
bool mission_audio::speech(std::string_view const path){
  if(!speech_enabled)return true; // Original disabled speech still advances the portrait.
  for(size_t slot{47};slot<57;++slot)if(!bank.playing(slot)){
    for(auto const volume:{"speech","simvoice"})if(auto const *entry{resources.archives().at(volume).find(path)}){
      bank.bind(slot,resources.archives().at(volume).read(*entry));auto const accepted{bank.play(slot)};if(observe)observe({audio_event_kind::speech,slot,path,accepted});return accepted;
    }
    return false;
  }
  return false;
}
void mission_audio::stop(size_t const id){bank.stop(id);if(observe)observe({audio_event_kind::stop,id,{}});}
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
    if(music)music->tick();
    auto const next{++ticks*11025/120};auto data{mixer.advance(static_cast<uint16_t>((next-sample_frames)*2))};output.insert(output.end(),data.begin(),data.end());
    auto const next_output{ticks*44100/120};auto const frames{static_cast<size_t>(next_output-output_frames)};
    auto fm{music?music->render(frames):std::vector<int16_t>(frames*2)};
    // Hold the 11025 Hz DAC value for four output samples. One source-frame
    // transport delay avoids reading ahead across timer boundaries.
    for(size_t f{};f<frames;++f){auto const absolute{output_frames+f};
      if(absolute%4==0 && absolute>=4){auto const index{absolute/4-1};if(index>=sample_frames&&index<next){held_pcm={data[static_cast<size_t>(index-sample_frames)*2],data[static_cast<size_t>(index-sample_frames)*2+1]};}}
      for(size_t c{};c<2;++c)mixed_output.push_back(static_cast<int16_t>((static_cast<int32_t>(fm[f*2+c])+(static_cast<int32_t>(held_pcm[c])-128)*256)/2));
    }
    if(!data.empty())held_pcm={data[data.size()-2],data.back()};
    sample_frames=next;output_frames=next_output;
  }
}
void mission_audio::advance_to(uint64_t const target){if(target>ticks)advance(static_cast<unsigned>(target-ticks));}
std::vector<uint8_t> mission_audio::take_samples(){mixed_output.clear();return std::exchange(output,{});}
std::vector<int16_t> mission_audio::take_output(){output.clear();return std::exchange(mixed_output,{});}
pcm_sound_bank const &mission_audio::state()const{return bank;}
} // namespace earthsiege
