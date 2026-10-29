#pragma once
#include "resources/binary_reader.h"
#include <array>
#include <optional>

namespace earthsiege {
struct midi_sample {
  uint32_t key{},volume{},pan{},flags{},offset{};
  int32_t note_off_marker{};
  uint16_t loops{};
  bytes samples;
};
struct pcm_midi_backend {
  virtual ~pcm_midi_backend()=default;
  virtual auto start(midi_sample const&)->int32_t=0;
  virtual void stop(int32_t)=0;
  virtual void volume(int32_t,uint32_t)=0;
  virtual void pan(int32_t,uint32_t)=0;
};
class hmi_pcm {
  pcm_midi_backend &backend;
  std::array<std::optional<midi_sample>,128> samples;
  // Preserve physical ring slots: the original removal search starts at slot
  // zero, not at the oldest entry, which matters after wraparound.
  std::array<std::array<int32_t,4>,32> slots;
  uint32_t head_{},tail_{},count_{},volume_{127},pan_{128};
  auto remove(std::optional<int32_t> handle,std::optional<uint8_t> note,std::optional<uint8_t> channel)->int32_t;
public:
  hmi_pcm(bytes bank,pcm_midi_backend &output);
  void midi(bytes message);
  void complete(int32_t handle);
  auto voices()const->decltype(slots)const&{return slots;}
  auto state()const->std::array<uint32_t,5>{return {head_,tail_,count_,volume_,pan_};}
};
}
