#include "effect_lighting.h"
#include <algorithm>
#include <stdexcept>
#include "simulation/fixed_math.h"
#include "simulation/model_math.h"

namespace earthsiege {

effect_lighting::effect_lighting(std::span<light const> const fixed, int32_t const scale, int32_t const offset)
  : default_attenuation{offset >> 5}, default_scale{scale >> 5} {
  if(fixed.size() > persistent.size()) throw std::invalid_argument{"too many persistent lights"};
  std::ranges::copy(fixed, persistent.begin());
  for(size_t index{0}; index != fixed.size(); ++index) active.add(persistent[index]);
}

std::optional<size_t> effect_lighting::add(vec3<int32_t> const &position) {
  for(size_t slot{0}; slot != sources.size(); ++slot) {
    if(sources[slot]) continue;
    sources[slot].emplace(source{.position{vec3<int32_t>{position}}, .intensity{255}, .attenuation{default_attenuation}, .scale{default_scale}, .rendered{}});
    order[count++] = slot;
    return slot;
  }
  return std::nullopt;
}

void effect_lighting::release(source &value) {
  if(!value.rendered) return;
  active.remove(*value.rendered);
  value.rendered.reset();
}

void effect_lighting::remove(size_t const slot) {
  auto &entry{sources.at(slot)};
  if(!entry) return;
  release(*entry);
  auto const found{std::find(order.begin(), order.begin() + static_cast<std::ptrdiff_t>(count), slot)};
  *found = order[--count];
  entry.reset();
}

void effect_lighting::intensity(size_t const slot, int16_t const value) { sources.at(slot).value().intensity = value; selection_intensity[slot] = value; }

void effect_lighting::attenuation(size_t const slot, int32_t const scale, int32_t const offset) {
  auto &entry{sources.at(slot).value()};
  entry.scale = scale >> 5;
  entry.attenuation = offset >> 5;
}

std::span<light const> effect_lighting::select(vec3<int32_t> const &object, int32_t const radius) {
  for(size_t index{0}; index != count; ++index) {
    auto &entry{sources[order[index]].value()};
    // Native zero-intensity sources skip selection, retaining any previous
    // renderer registration until their owner releases them.
    auto const intensity{selection_intensity[order[index]]};
    if(intensity == 0) continue;
    vec3<int32_t> delta;
    for(unsigned int axis{0}; axis != 3; ++axis) delta[axis] = wrap_long(static_cast<int64_t>(object[axis]) - entry.position[axis]);
    auto const distance{fast_distance(delta)};
    auto const reach{wrap_long(wrap_long(static_cast<int64_t>(entry.scale) * 32 * entry.intensity) / 10LL + static_cast<int64_t>(entry.attenuation) * 32)};
    if(distance >= reach) { release(entry); continue; }
    light value{.kind{light_kind::positional}, .intensity{intensity}, .position{vec3<int32_t>{entry.position}}, .direction{}, .attenuation{entry.attenuation * 32}, .scale{entry.scale * 32}};
    auto const coarse_distance{distance >> 5};
    if(coarse_distance != 0 && wrap_word(direction_angle(coarse_distance, radius >> 5)) < 8000) {
      auto const denominator{wrap_long(static_cast<int64_t>(coarse_distance) + entry.attenuation)};
      if(denominator == 0) throw std::domain_error{"dynamic light has zero attenuation denominator"};
      auto const weight{std::min(255, wrap_long(static_cast<int64_t>(entry.scale) * 256) / denominator)};
      value = light{.kind{light_kind::directional}, .intensity{wrap_long(static_cast<int64_t>(intensity) * weight) >> 8}};
      for(unsigned int axis{0}; axis != 3; ++axis) value.direction[axis] = wrap_word(wrap_long(static_cast<int64_t>(delta[axis]) * model_normal_one) / distance);
    }
    if(!entry.rendered || entry.rendered->kind != value.kind) {
      release(entry);
      entry.rendered = value;
      active.add(*entry.rendered);                                         // full list rejects this allocation; no retry until it is released
    } else {
      *entry.rendered = value;
    }
  }
  return current();
}

std::span<light const> effect_lighting::current() {
  auto const registered{active.lights()};
  for(size_t index{0}; index != registered.size(); ++index) selected[index] = *registered[index];
  return {selected.data(), registered.size()};
}

} // namespace earthsiege
