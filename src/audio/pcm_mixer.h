#pragma once
#include "audio/pcm_sound.h"
#include <span>

namespace earthsiege {
/// Archived tr8m/tr8st arithmetic: quantize gain, signed byte intermediate,
/// then saturate each voice into the existing unsigned-byte destination.
uint8_t mix_pcm8(uint8_t sample,uint8_t background,uint16_t volume,uint16_t balance=32767);
class pcm_mixer {
  pcm_sound_bank &bank;
  bool stereo;
  std::array<uint8_t,4096> buffer;
  uint32_t dma_position{},previous_position{};
  std::vector<size_t> draining;
public:
  explicit pcm_mixer(pcm_sound_bank &sounds,bool stereo_output=true);
  /// Advance virtual DMA by a byte count, returning bytes already played.
  /// The original mixer refills the ring after that hardware boundary.
  std::vector<uint8_t> advance(uint16_t byte_count);
  std::array<uint8_t,4096> const &ring()const;
};
} // namespace earthsiege
