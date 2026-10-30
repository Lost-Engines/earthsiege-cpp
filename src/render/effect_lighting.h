#pragma once
#include <array>
#include <optional>
#include "render/model_lighting.h"

namespace earthsiege {

/// Own dynamic light sources and the renderer's persistent registration order.
/// Selection adapts each source to the size/distance of the object being drawn.
class effect_lighting {
  struct source {
    vec3<int32_t> position;
    int16_t intensity{255};
    int32_t attenuation{0};                                               // stored in units of 32 world coordinates
    int32_t scale{0};
    std::optional<light> rendered;
  };
  std::array<std::optional<source>, effect_light_limit> sources;
  std::array<int32_t, effect_light_limit> selection_intensity{};
  std::array<size_t, effect_light_limit> order{};
  size_t count{0};
  int32_t default_attenuation;
  int32_t default_scale;
  std::array<light, renderer_light_limit> persistent;
  light_list active;
  std::array<light, renderer_light_limit> selected;
  void release(source &value);

public:
  effect_lighting(std::span<light const> persistent, int32_t scale = 3000, int32_t attenuation = 2000);
  effect_lighting(effect_lighting const &) = delete;
  effect_lighting(effect_lighting &&) = delete;
  effect_lighting &operator=(effect_lighting const &) = delete;
  effect_lighting &operator=(effect_lighting &&) = delete;
  std::optional<size_t> add(vec3<int32_t> const &position);
  void remove(size_t slot);
  void intensity(size_t slot, int16_t value);
  void attenuation(size_t slot, int32_t scale, int32_t offset);
  std::span<light const> current();
  std::span<light const * const> registered()const{return active.lights();}
  std::span<light const> select(vec3<int32_t> const &object, int32_t radius);
};

} // namespace earthsiege
