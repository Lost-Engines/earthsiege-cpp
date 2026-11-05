#pragma once

#include <cstdint>
#include <memory>
#include <span>

namespace earthsiege {

/// Miniaudio transport for the combined 44100 Hz signed stereo stream.
class audio_output {
  struct implementation;
  std::unique_ptr<implementation> state;

public:
  audio_output();
  ~audio_output();
  audio_output(audio_output const &) = delete;
  auto operator=(audio_output const &)->audio_output & = delete;
  void queue(std::span<int16_t const> samples);
  void pause(bool paused);
  void clear();
};

} // namespace earthsiege
