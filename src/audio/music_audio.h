#pragma once
#include "music_player.h"
#include "pcm_sound.h"
#include "resources/resource_store.h"
namespace earthsiege {
// Music protocol plus OPL synthesis, sharing the game's 32 digital voices.
class music_audio:private pcm_midi_backend {
  pcm_sound_bank &bank;
  music_player player;
  struct chip_state;
  std::unique_ptr<chip_state> chip;
  auto start(midi_sample const&)->int32_t override;
  void stop(int32_t)override;
  void volume(int32_t,uint32_t)override;
  void pan(int32_t,uint32_t)override;
public:
  music_audio(resource_store const&,pcm_sound_bank&,bool opl3=true);
  ~music_audio();
  void load(bytes song);
  void play();
  void stop();
  bool playing()const;
  void set_volume(uint8_t value,uint8_t master=90);
  void tick();
  auto render(size_t frames)->std::vector<int16_t>;
};
}
