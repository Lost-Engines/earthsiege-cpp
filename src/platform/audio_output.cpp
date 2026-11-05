#include "audio_output.h"
#include <stdexcept>
#include <string>
#include <miniaudio.h>
#include "pcm_queue.h"

namespace earthsiege {

struct audio_output::implementation {
  pcm_queue samples;
  ma_device device{};

  implementation() {
    auto config{ma_device_config_init(ma_device_type_playback)};
    config.playback.format = ma_format_s16;
    config.playback.channels = 2;
    config.sampleRate = 44100;
    config.periodSizeInFrames = 256;
    config.pUserData = this;
    config.dataCallback = [](ma_device *device, void *output, void const *, ma_uint32 frames) {
      auto &self{*static_cast<implementation *>(device->pUserData)};
      self.samples.read({static_cast<int16_t *>(output), static_cast<size_t>(frames) * 2});
    };
    auto const result{ma_device_init(nullptr, &config, &device)};
    if(result != MA_SUCCESS) throw std::runtime_error{std::string{"cannot open PCM output: "} + ma_result_description(result)};
  }
  ~implementation() { ma_device_uninit(&device); }
};

audio_output::audio_output() : state{std::make_unique<implementation>()} {
  auto const result{ma_device_start(&state->device)};
  if(result != MA_SUCCESS) throw std::runtime_error{std::string{"cannot start PCM output: "} + ma_result_description(result)};
}
audio_output::~audio_output() = default;
void audio_output::queue(std::span<int16_t const> samples) { state->samples.queue(samples); }
void audio_output::pause(bool paused) { state->samples.pause(paused); }
void audio_output::clear() { state->samples.clear(); }

} // namespace earthsiege
