#pragma once
#include <cstdint>
#include <span>
#include <SDL.h>

namespace earthsiege {
/// Host transport for the combined 44100 Hz signed stereo stream.
class audio_output {
  SDL_AudioDeviceID device{};
public:
  audio_output();
  ~audio_output();
  audio_output(audio_output const &)=delete;
  audio_output &operator=(audio_output const &)=delete;
  void queue(std::span<int16_t const> samples);
  void pause(bool paused);
  void clear();
};
} // namespace earthsiege
