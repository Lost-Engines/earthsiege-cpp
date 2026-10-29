#pragma once
#include "audio/pcm_mixer.h"
#include "audio/music_audio.h"
#include "shell/shell_animation.h"
namespace earthsiege {
/// GO music and TTM PCM objects sharing the original digital mixer.
class shell_audio:public shell_animation_sound {
 resource_store const &resources;
 pcm_sound_bank bank;
 pcm_mixer mixer{bank};
 std::unique_ptr<music_audio> music;
 int track{-1},wanted{-1},gain{100},fade_next{100};
 bool music_enabled{true},effects_enabled{true},suspended{};
 uint64_t ticks{},pcm_frames{},output_frames{};
 std::array<uint8_t,2> held_pcm{128,128};
 std::vector<int16_t> output;
 void start_song();
public:
 struct observer {
  virtual ~observer()=default;
  // 1 load ID, 2 stop ID, 3 play ID, 4 volume, 5 loop count.
  virtual void event(uint64_t tick,uint8_t kind,int value)=0;
 };
 observer *record{};
 explicit shell_audio(resource_store const &r):resources{r}{}
 void song(int id);
 void region(unsigned id);
 void enabled(bool music,bool effects);
 void suspend();
 void resume();
 shell_animation_sound *effects(){return effects_enabled&&!suspended?this:nullptr;}
 bool load(std::string_view name)override;
 void volume(uint8_t value)override;
 void play()override;
 void clear_sample();
 void advance(unsigned timer_ticks=1);
 std::vector<int16_t> take_output();
 int song_id()const{return track;}
 int requested_song()const{return wanted;}
 int song_gain()const{return gain;}
 pcm_sound_bank const &samples()const{return bank;}
};
}
