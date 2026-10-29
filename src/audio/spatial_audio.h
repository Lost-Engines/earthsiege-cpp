#pragma once
#include "simulation/fixed_math.h"
#include "simulation/model_math.h"
#include <array>
#include <optional>
#include <vector>

namespace earthsiege {
struct positional_audio_state {std::optional<uint8_t> gain;uint16_t pan{32768};};
positional_audio_state spatial_audio_gain(vec3<int32_t> const &position,model_transform const &listener,uint8_t base,int32_t cutoff,unsigned int shift,uint16_t previous_pan,bool enabled=true);
struct positional_sound {size_t id{};vec3<int32_t> position{};};
/// Original first-nonzero-distance source selection and ten-flush listener clock.
class sound_sources {
  std::array<int32_t,60> pending{};
  std::array<vec3<int32_t>,60> positions{};
  unsigned int cadence{10};
  vec3<int32_t> origin{};
public:
  explicit sound_sources(vec3<int32_t> const &initial_listener={}):origin{initial_listener}{}
  void submit(size_t id,vec3<int32_t> const &position,vec3<int32_t> const &listener,bool enabled=true);
  std::vector<positional_sound> flush(vec3<int32_t> const &listener,bool enabled=true);
  unsigned int remaining()const;
  vec3<int32_t> listener()const;
  std::array<int32_t,60> const &distances()const;
  std::array<vec3<int32_t>,60> const &sources()const;
};
} // namespace earthsiege
