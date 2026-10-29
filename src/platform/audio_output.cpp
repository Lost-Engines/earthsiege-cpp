#include "audio_output.h"
#include <iostream>
#include <stdexcept>

namespace earthsiege {
audio_output::audio_output(){
  if(SDL_InitSubSystem(SDL_INIT_AUDIO)!=0)throw std::runtime_error{SDL_GetError()};
  SDL_AudioSpec desired{};desired.freq=44100;desired.format=AUDIO_S16SYS;desired.channels=2;desired.samples=256;
  device=SDL_OpenAudioDevice(nullptr,0,&desired,nullptr,0);
  if(!device){SDL_QuitSubSystem(SDL_INIT_AUDIO);throw std::runtime_error{SDL_GetError()};}
  SDL_PauseAudioDevice(device,0);
}
audio_output::~audio_output(){SDL_CloseAudioDevice(device);SDL_QuitSubSystem(SDL_INIT_AUDIO);}
void audio_output::queue(std::span<int16_t const> const samples){if(!samples.empty() && SDL_QueueAudio(device,samples.data(),static_cast<uint32_t>(samples.size_bytes()))!=0)throw std::runtime_error{SDL_GetError()};}
void audio_output::pause(bool const paused){SDL_PauseAudioDevice(device,paused?1:0);}
void audio_output::clear(){SDL_ClearQueuedAudio(device);}
} // namespace earthsiege
