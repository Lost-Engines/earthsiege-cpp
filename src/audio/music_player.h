#pragma once
#include "hmi_fm.h"
#include "hmi_pcm.h"
#include "midi_gain.h"
#include "music_loops.h"
#include <memory>
namespace earthsiege {
// Complete HMI music protocol, with explicit chip and digital-output boundaries.
class music_player {
  std::array<uint32_t,2> devices;
  hmi_fm fm;
  hmi_pcm pcm;
  midi_gain gain;
  std::optional<hmp_song> song;
  std::unique_ptr<hmp_player> scheduler;
  bool playing_{};
  std::optional<music_loop_state> loops;
  void dispatch(midi_packet const &packet);
  void cleanup();
public:
  music_player(bool opl3,bytes melodic,bytes drums,bytes digital_bank,pcm_midi_backend &backend);
  void load(bytes hmp);
  void set_loops(uint16_t count);
  void play();
  void stop();
  bool playing()const{return playing_;}
  void tick();
  void set_master(uint8_t value);
  void complete_sample(int32_t handle);
  auto take_registers()->std::vector<opl_write>;
};
}
