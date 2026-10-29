#pragma once
#include "resources/binary_reader.h"
#include <array>
#include <optional>
#include <vector>

namespace earthsiege {
struct pcm_sound_state {
  bool registered{},positional{},play_silent{};
  uint8_t volume{100};
  uint16_t spatial_volume{},pan{32768},loops{},current_loops{},marker{65535};
  uint32_t rate{};
  bool rate_override{};
  int32_t loop_start{-1},loop_end{-1},handle{-1},error{};
  bytes samples;
};
struct pcm_voice_state {
  uint16_t flags{},volume{},loops{},key{};
  uint32_t rate{65536};
  uint16_t pan{32768};
  int32_t loop_start{-1},loop_end{-1};
  bytes samples;
  uint32_t cursor{},remaining{},consumed{};
  uint16_t output_cursor{},drain_remaining{};
};
struct pcm_start {
  size_t slot{},length{};
  uint16_t loops{},volume{},flags{};
  std::optional<uint32_t> rate;
  std::optional<uint16_t> pan;
  int32_t loop_start{-1},loop_end{-1};
};
/// The original SoundManager's PCM objects and HMI's 32-voice allocator.
/// Device stop and end notifications are explicit boundaries. Sample storage
/// must remain valid until all voices referencing it have ended.
class pcm_sound_bank {
  friend class pcm_mixer;
  std::array<pcm_sound_state,60> sounds{};
  std::array<pcm_voice_state,32> channels{};
  uint8_t master{90};
  uint32_t reported_volume(size_t slot)const;
  int32_t allocate(pcm_start const &request);
public:
  // Optional recording boundary; used by original-code comparison fixtures.
  struct observer {
    virtual ~observer()=default;
    virtual void start(pcm_start const &){};
    virtual void stop(size_t){};
  };
  observer *record{};
  void bind(size_t slot,bytes samples);
  void set_master(uint8_t volume);
  void set_volume(size_t slot,int8_t volume);
  void set_spatial_volume(size_t slot,int8_t volume);
  void set_positioned(size_t slot,bool enabled,uint16_t pan);
  void set_rate(size_t slot,uint32_t rate);
  void set_loops(size_t slot,int32_t end,int32_t start,uint16_t count);
  void allow_silent(size_t slot,bool enabled);
  bool play(size_t slot);
  bool playing(size_t slot)const;
  void stop(size_t slot);
  void suspend(size_t slot); // Original device stop which retains the handle.
  void complete(size_t handle);
  void voice_volume(size_t handle,uint16_t volume);
  pcm_sound_state const &sound(size_t slot)const;
  std::array<pcm_voice_state,32> const &voices()const;
};
} // namespace earthsiege
