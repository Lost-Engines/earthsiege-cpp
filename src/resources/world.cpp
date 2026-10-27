#include "world.h"
#include <stdexcept>

namespace earthsiege {

zone_resource read_zone(bytes const source) {
  binary_reader file{source, "zone placements"};
  zone_resource result{.stored_terrain{file.integer<uint16_t>()}, .placements{}};
  auto const count{file.integer<uint16_t>()};
  result.placements.reserve(count);
  for(unsigned int index{0}; index != count; ++index) {
    result.placements.push_back(zone_placement{
      .shape{file.integer<uint16_t>()},
      .position{file.integer<int32_t>(), file.integer<int32_t>(), file.integer<int32_t>()},
      .angles{file.integer<uint16_t>(), file.integer<uint16_t>(), file.integer<uint16_t>()},
    });
  }
  file.require_end();
  return result;
}

world_environment read_world_environment(bytes const source) {
  binary_reader file{source, "world environment"};
  world_environment result;
  for(auto &value : result.sky_ground_parameters) value = file.integer<int16_t>();
  for(auto &band : result.distance_bands) {
    auto const count{file.integer<uint32_t>()};
    if(count > file.remaining() / sizeof(int32_t)) file.fail("distance band exceeds descriptor");
    band.reserve(count);
    for(unsigned int index{0}; index != count; ++index) band.push_back(file.integer<int32_t>());
  }
  result.shade_levels = file.integer<int16_t>();
  auto const count{file.integer<int16_t>()};
  if(count < 0) file.fail("negative palette range count");
  auto const range{[&file] {
    return palette_range{.first{file.integer<int16_t>()}, .count{file.integer<int16_t>()}};
  }};
  for(int index{0}; index != count; ++index) result.near_ranges.push_back(range());
  result.stored_far_range_count = file.integer<int16_t>();
  for(int index{0}; index != count; ++index) result.far_ranges.push_back(range());
  for(auto &entry : result.blend_ranges) entry = range();
  for(auto &value : result.effect_parameters) value = file.integer<int16_t>();
  for(auto &value : result.model_distance_offsets) value = file.integer<int32_t>();
  for(auto &name : result.resource_names) name = file.cstring();
  file.require_end();
  return result;
}

bool terrain_metadata::used_by_instance() const {
  return sort_endpoints[0].z >= 0;
}

std::vector<terrain_metadata> read_terrain_metadata(bytes const source) {
  binary_reader file{source, "terrain metadata"};
  std::vector<terrain_metadata> result;
  while(file.remaining() != 0) {
    terrain_metadata entry;
    for(auto &point : entry.sort_endpoints) {
      for(unsigned int axis{0}; axis != 3; ++axis) point[axis] = file.integer<int32_t>();
    }
    result.push_back(entry);
  }
  return result;
}

world_selection select_world(mission_header const &header) {
  /// Each terrain has three palettes, each shared by two environment variants
  int constexpr variants_per_terrain{6};
  int constexpr environments_per_palette{2};
  auto const palette{static_cast<int>(header.terrain) * variants_per_terrain + header.palette_variant * environments_per_palette};
  auto const environment{palette + header.world_variant};
  if(palette < 0 || palette >= 24 || environment < 0 || environment >= 24) throw std::out_of_range{"mission world selection"};
  return world_selection{.palette{palette}, .environment{environment}};
}

std::string_view terrain_name(terrain_kind const terrain) {
  switch(terrain) {
  case terrain_kind::desert: return "DESERT";
  case terrain_kind::temperate: return "TEMPER";
  case terrain_kind::ice: return "ICE";
  case terrain_kind::urban: return "URBAN";
  }
  throw std::out_of_range{"unknown terrain kind"};
}

} // namespace earthsiege
