#pragma once
#include "music_player.h"
#include "music_settings.h"
#include "midi_receiver.h"
#include "pcm_sound.h"
#include "resources/resource_store.h"
namespace earthsiege {
// Original music protocol and physical synthesizer, sharing 32 digital voices.
class music_audio:private pcm_midi_backend {
  resource_store const &resources;
  pcm_sound_bank &bank;
  std::unique_ptr<midi_receiver> midi;
  std::unique_ptr<music_player> player;
  struct chip_state;
  std::unique_ptr<chip_state> chip;
  std::vector<uint8_t> song;
  uint16_t repeats{255};
  uint8_t volume_{100},master_{90};
  auto start(midi_sample const&)->int32_t override;
  void stop(int32_t)override;
  void volume(int32_t,uint32_t)override;
  void pan(int32_t,uint32_t)override;
  void send(std::span<uint8_t const>);
public:
  music_audio(resource_store const&,pcm_sound_bank&,music_settings const &settings={});
  ~music_audio();
  void configure(music_settings const &);
  void load(bytes song,uint16_t loops=255);
  void play();
  void stop();
  bool playing()const;
  void set_volume(uint8_t value,uint8_t master=90);
  void tick();
  auto render(size_t frames)->std::vector<int16_t>;
};
}
