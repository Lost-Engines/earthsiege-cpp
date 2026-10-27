#include "model_lighting.h"
#include <algorithm>
#include <stdexcept>
#include "simulation/fixed_math.h"
#include "simulation/model_math.h"

namespace earthsiege {

bool light_list::add(light const &source) {
  /// Keep source identity and insertion order, including the original capacity
  if(count == sources.size() || std::ranges::find(lights(), &source) != lights().end()) return false;
  sources[count++] = &source;
  return true;
}

void light_list::remove(light const &source) {
  auto const found{std::ranges::find(lights(), &source)};
  if(found == lights().end()) return;
  sources[static_cast<size_t>(found - lights().begin())] = sources[--count];
  sources[count] = nullptr;
}

std::span<light const * const> light_list::lights() const {
  return {sources.data(), count};
}

light default_world_light() {
  /// Original persistent light after quantized Euler rotation at startup
  return {
    .kind{light_kind::directional},
    .intensity{256},
    .position{0, 0, 0},
    .direction{-1099, -984, -1421},
  };
}

light transform_light(light const &source, model_transform const &inverse_object) {
  /// Convert world light vectors to object coordinates without renormalizing
  auto result{source};
  result.position = rotate_vector(source.position, inverse_object);
  auto const direction{rotate_vector(vec3<int32_t>{source.direction}, inverse_object)};
  for(unsigned int axis{0}; axis != 3; ++axis) {
    result.position[axis] = wrap_long(static_cast<int64_t>(result.position[axis]) + inverse_object.translation[axis]);
    result.direction[axis] = wrap_word(direction[axis]);
  }
  return result;
}

uint8_t light_intensity(std::span<light const> const lights, vec3<int32_t> const &point, vec3<int16_t> const &normal, lighting_surface const surface) {
  /// Reproduce native signed accumulation, incidence and palette-intensity clamp
  auto const dot{[&normal](auto const &vector){
    int64_t value{0};
    for(unsigned int axis{0}; axis != 3; ++axis) value += static_cast<int64_t>(normal[axis]) * vector[axis];
    return wrap_long(value);
  }};
  int32_t total{0};
  for(auto const &source : lights) {
    int32_t contribution{0};
    switch(source.kind) {
    case light_kind::ambient:
      contribution = source.intensity;
      break;
    case light_kind::directional:
      if(surface == lighting_surface::unoriented) {
        contribution = source.intensity;
      } else {
        auto facing{dot(source.direction)};
        if(surface == lighting_surface::model) facing = wrap_long(static_cast<int64_t>(facing) - model_normal_one * model_normal_one) >> 1;
        if(facing < 0) contribution = -(wrap_long(static_cast<int64_t>(source.intensity) * facing) >> 22);
      }
      break;
    case light_kind::positional:
      {
        vec3<int32_t> delta;
        for(unsigned int axis{0}; axis != 3; ++axis) delta[axis] = wrap_long(static_cast<int64_t>(source.position[axis]) - point[axis]);
        auto const distance{fast_distance(delta)};
        auto const facing{surface == lighting_surface::unoriented ? 1 : dot(delta)};
        if(facing <= 0) break;
        auto const denominator{wrap_long(static_cast<int64_t>(source.attenuation) + distance)};
        if(distance == -1 || denominator == 0) throw std::domain_error{"positional light has zero attenuation denominator"};
        int32_t const incidence{surface == lighting_surface::unoriented ? model_normal_one - 1 : facing / (distance + 1)};
        auto const attenuation{wrap_long(static_cast<int64_t>(source.scale) * incidence) / denominator};
        contribution = wrap_long(static_cast<int64_t>(source.intensity) * attenuation) >> 11;
      }
      break;
    }
    total = wrap_long(static_cast<int64_t>(total) + contribution);
  }
  return static_cast<uint8_t>(std::min(255, total));                               // native code has no lower clamp
}

} // namespace earthsiege
