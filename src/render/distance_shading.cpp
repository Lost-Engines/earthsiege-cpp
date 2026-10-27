#include "distance_shading.h"
#include <algorithm>
#include <stdexcept>
#include <utility>
#include "simulation/fixed_math.h"

namespace earthsiege {
namespace {

int16_t constexpr terrain_class{0};
int16_t constexpr ground_target_class{5};
int16_t constexpr effect_class{8};
int16_t constexpr large_model_radius{5000};

std::vector<palette_range> interpolate_ranges(palette_range const near, palette_range const far, int const levels) {
  if(levels <= 0) throw std::invalid_argument{"nonpositive distance shade levels"};
  std::vector<palette_range> result;
  auto const near_last{wrap_word(near.first + near.count - 1)};
  auto const far_last{wrap_word(far.first + far.count - 1)};
  if(levels > 1) {
    auto const first_step{wrap_word((far.first - near.first + 1) * 256 / (levels - 1))};
    auto const last_step{wrap_word((far_last - near_last + 1) * 256 / (levels - 1))};
    for(int index{0}; index < levels - 1; ++index) {
      auto const first{wrap_word(near.first + ((index * first_step) >> 8))};
      auto const count{wrap_word(near_last + ((index * last_step) >> 8) - first + 1)};
      result.push_back(palette_range{.first{first}, .count{count}});
    }
  }
  result.push_back(far);
  return result;
}

void append_range(std::vector<uint16_t> &target, int const first, int const count) {
  for(int index{0}; index < count; ++index) target.push_back(static_cast<uint16_t>(first + index));
}

} // anonymous namespace

distance_shade_bindings read_distance_shade_bindings(bytes const source) {
  binary_reader file{source, "distance shade bindings"};
  distance_shade_bindings result(file.integer<uint8_t>());
  auto const count{file.integer<uint8_t>()};
  for(unsigned int index{0}; index != count; ++index) {
    auto const slot{file.integer<uint8_t>()};
    auto const pairs{file.integer<uint8_t>()};
    auto &entry{result.at(slot)};
    entry.clear();
    for(unsigned int pair{0}; pair != pairs; ++pair) entry.push_back(shade_binding{.palette_slot{file.integer<uint8_t>()}, .environment_column{file.integer<uint8_t>()}});
  }
  file.require_end();
  return result;
}

distance_shading::distance_shading(world_environment const &source, distance_shade_bindings mapping) : environment{source}, bindings{std::move(mapping)} {
  if(source.near_ranges.size() != source.far_ranges.size()) throw std::invalid_argument{"distance palette columns differ"};
  for(size_t index{0}; index != source.near_ranges.size(); ++index) bands.push_back(interpolate_ranges(source.near_ranges[index], source.far_ranges[index], source.shade_levels));
  highlights = interpolate_ranges(source.blend_ranges[0], source.blend_ranges[1], source.shade_levels);
}

size_t distance_shading::band(int32_t distance, int16_t const native_class, int16_t const radius) const {
  if(native_class == ground_target_class) distance = std::max(0, wrap_long(static_cast<int64_t>(distance) + environment.model_distance_offsets[radius >= large_model_radius ? 1 : 0]));
  auto const &distances{environment.distance_bands[native_class == terrain_class ? 1 : 0]};
  if(distances.empty()) throw std::runtime_error{"empty distance bands"};
  size_t index{0};
  while(index + 1 < distances.size() && static_cast<uint32_t>(distance) >= static_cast<uint32_t>(distances[index])) ++index;
  return index;
}

bool distance_shading::culled(int32_t distance, int16_t const native_class, int16_t const radius) const {
  if(native_class == ground_target_class || native_class == effect_class) {
    auto const offset{environment.model_distance_offsets[native_class == ground_target_class && radius >= large_model_radius ? 1 : 0]};
    distance = std::max(0, wrap_long(static_cast<int64_t>(distance) + offset));
  }
  auto const &distances{environment.distance_bands[native_class == terrain_class ? 1 : 0]};
  if(distances.empty()) throw std::runtime_error{"empty distance bands"};
  return static_cast<uint32_t>(distance) > static_cast<uint32_t>(distances.back());
}

void distance_shading::apply(palette &target, int32_t const distance, int16_t const native_class, int16_t const radius, bool const split_highlights) const {
  auto const &mapping{bindings.at(static_cast<size_t>(native_class))};
  if(mapping.empty()) return;
  auto const level{band(std::max(0, distance), native_class, radius)};
  for(auto const &binding : mapping) {
    auto const range{bands.at(binding.environment_column).at(level)};
    auto &ramp{target.shade_ramps.at(binding.palette_slot)};
    ramp.clear();
    if(split_highlights) {
      auto const trim{range.count >> 2};
      append_range(ramp, range.first, range.count - trim - 1);
      auto const highlight{highlights.at(level)};
      append_range(ramp, highlight.first + highlight.count - trim, trim + 1);
    } else append_range(ramp, range.first, range.count);
  }
}

} // namespace earthsiege
