#pragma once
#include <cstdint>
#include <span>
#include <SDL.h>

namespace earthsiege {
/// Host transport for the original 11025 Hz unsigned stereo PCM stream.
class audio_output {
  SDL_AudioDeviceID device{};
public:
  audio_output();
  ~audio_output();
  audio_output(audio_output const &)=delete;
  audio_output &operator=(audio_output const &)=delete;
  void queue(std::span<uint8_t const> samples);
  void pause(bool paused);
  void clear();
};
} // namespace earthsiege
