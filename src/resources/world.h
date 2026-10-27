#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include "resources/mission_script.h"

namespace earthsiege {

struct zone_placement {
  uint16_t shape{0};
  vec3<int32_t> position;
  vec3<uint16_t> angles;
};

struct zone_resource {
  uint16_t stored_terrain{0};                                                   // original loader reads this but selects terrain from the mission instead
  std::vector<zone_placement> placements;
};

struct palette_range {
  int16_t first{0};
  int16_t count{0};
};

struct background_style {
  int16_t horizon_band_width{0};
  int16_t sky_palette_start{0};
  int16_t sky_step{0};
  int16_t sky_bands{0};
  int16_t ground_palette_start{0};
  int16_t ground_step{0};
  int16_t ground_bands{0};
  int16_t horizon_offset{0};
};

struct world_environment {
  background_style background;
  std::array<int16_t, 4> atmosphere_parameters{};
  std::array<std::vector<int32_t>, 2> distance_bands;
  int16_t shade_levels{0};
  std::vector<palette_range> near_ranges;
  int16_t stored_far_range_count{0};                                            // original uses the near count for both arrays
  std::vector<palette_range> far_ranges;
  std::array<palette_range, 2> blend_ranges;
  std::array<int16_t, 2> effect_parameters{};
  std::array<int32_t, 2> model_distance_offsets{};
  std::array<std::string, 5> resource_names;
};

struct terrain_metadata {
  std::array<vec3<int32_t>, 2> sort_endpoints;                                  // local segment used by the whole-scene painter tree
  bool used_by_instance() const;
};

struct world_selection {
  int palette{0};
  int environment{0};
};

zone_resource read_zone(bytes source);
world_environment read_world_environment(bytes source);
std::vector<terrain_metadata> read_terrain_metadata(bytes source);
world_selection select_world(mission_header const &header);
std::string_view terrain_name(terrain_kind terrain);

} // namespace earthsiege
