#pragma once
#include "audio/pcm_mixer.h"
#include "audio/spatial_audio.h"
#include "resources/resource_store.h"
#include "simulation/sound_request.h"

namespace earthsiege {
/// Simulation sound resources and deterministic 120 Hz digital output clock.
class mission_audio {
  resource_store const &resources;
  pcm_sound_bank bank;
  pcm_mixer mixer{bank};
  sound_sources sources;
  std::array<vec3<int32_t>,60> positions{};
  std::array<bool,60> positioned{};
  uint64_t ticks{},sample_frames{};
  std::vector<uint8_t> output;
  model_transform listener()const;
  void position(size_t id,vec3<int32_t> const &where);
  void update_gain(size_t id);
public:
  explicit mission_audio(resource_store const &resources);
  void dispatch(sound_request const &request, vec3<int32_t> const &listener);
  bool play(size_t id);
  bool play_at(size_t id,vec3<int32_t> const &where);
  bool speech(std::string_view path);
  void stop(size_t id);
  bool playing(size_t id)const;
  void submit_position(size_t id,vec3<int32_t> const &where,vec3<int32_t> const &listener);
  void flush_positions(vec3<int32_t> const &listener);
  void advance(unsigned int timer_ticks=3);
  std::vector<uint8_t> take_samples();
  pcm_sound_bank const &state()const;
};
} // namespace earthsiege
