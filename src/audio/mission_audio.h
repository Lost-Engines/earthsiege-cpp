#pragma once
#include "audio/pcm_mixer.h"
#include "audio/music_audio.h"
#include "audio/combat_music.h"
#include "audio/audio_controls.h"
#include "audio/spatial_audio.h"
#include "resources/resource_store.h"
#include "simulation/sound_request.h"

namespace earthsiege {
/// Simulation sound resources and deterministic 120 Hz digital output clock.
enum class audio_event_kind { play,stop,speech,music_load,music_play,music_stop,music_volume };
struct audio_event {audio_event_kind kind;size_t id{};std::string_view path;bool accepted{true};};
using audio_event_sink=std::function<void(audio_event const &)>;

class mission_audio {
  resource_store const &resources;
  music_settings music_config;
  pcm_sound_bank bank;
  pcm_mixer mixer{bank};
  std::unique_ptr<music_audio> music;
  combat_music_state music_state;
  music_playback playback;
  std::array<std::string,6> songs;
  sound_sources sources;
  std::array<servo_sound_state,2> servos;
  target_sound_state target_tone;
  std::array<vec3<int32_t>,60> positions{};
  std::array<bool,60> positioned{};
  bool music_enabled{true},effects_enabled{true},speech_enabled{true};
  uint64_t ticks{},sample_frames{};
  std::vector<uint8_t> output;
  std::vector<int16_t> mixed_output;
  uint64_t output_frames{};
  std::array<uint8_t,2> held_pcm{128,128};
  model_transform listener()const;
  void position(size_t id,vec3<int32_t> const &where);
  void update_gain(size_t id);
public:
  audio_event_sink observe;
  explicit mission_audio(resource_store const &resources,music_settings const &settings={});
  void configure_music(music_settings const &settings){
    if(music)music->configure(settings);
    else music=std::make_unique<music_audio>(resources,bank,settings);
    music_config=settings;
  }
  bool preferences(bool music,bool effects,bool speech);
  void dispatch(sound_request const &request, vec3<int32_t> const &listener);
  void engine(bool external_view);
  void update_music(music_contacts const &contacts,random_state &random);
  bool play(size_t id);
  bool play_at(size_t id,vec3<int32_t> const &where);
  bool speech(std::string_view path);
  void stop(size_t id);
  bool playing(size_t id)const;
  void submit_position(size_t id,vec3<int32_t> const &where,vec3<int32_t> const &listener);
  void flush_positions(vec3<int32_t> const &listener);
  void advance(unsigned int timer_ticks=3);
  void advance_to(uint64_t timer_ticks);
  std::vector<uint8_t> take_samples();
  std::vector<int16_t> take_output();
  pcm_sound_bank const &state()const;
  combat_music_state const &music_status()const{return music_state;}
};
} // namespace earthsiege
