#pragma once
#include <filesystem>
#include <memory>
#include "midi_receiver.h"
namespace earthsiege {
class soundfont_synth final : public midi_receiver {
  struct implementation;
  std::unique_ptr<implementation> state;
public:
  soundfont_synth(std::filesystem::path const &, unsigned sample_rate);
  ~soundfont_synth() override;
  void short_message(uint32_t) override;
  void sysex(std::span<uint8_t const>) override;
  void render(std::span<float>) override;
};
} // namespace earthsiege
