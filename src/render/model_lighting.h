#pragma once
#include <array>
#include <cstdint>
#include <span>
#include "resources/model.h"

namespace earthsiege {

enum class light_kind {
  ambient,
  directional,
  positional,
};

enum class lighting_surface {
  model,
  terrain,
  unoriented,
};

struct light {
  light_kind kind{light_kind::ambient};
  int32_t intensity{0};
  vec3<int32_t> position;
  vec3<int16_t> direction;
  int32_t attenuation{0};
  int32_t scale{0};
};

inline size_t constexpr renderer_light_limit{10};
inline size_t constexpr effect_light_limit{20};

class light_list {
  std::array<light const *, renderer_light_limit> sources{nullptr};
  size_t count{0};

public:
  bool add(light const &source);
  void remove(light const &source);
  std::span<light const * const> lights() const;
};

light default_world_light();
light transform_light(light const &source, model_transform const &inverse_object);
uint8_t light_intensity(std::span<light const> lights, vec3<int32_t> const &point, vec3<int16_t> const &normal, lighting_surface surface = lighting_surface::model);

} // namespace earthsiege
