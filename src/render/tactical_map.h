#pragma once
#include <optional>
#include "render/model_renderer.h"
#include "simulation/scenery_world.h"

namespace earthsiege {
struct tactical_contact {
  int16_t kind{-1};                                                          // -1 waypoint, 0 HERC, 1 building, 2 flyer
  int16_t frame{}, size{};
  uint8_t side{};
  bool dead{}, hidden{}, detected{}, last_position_unavailable{true};
  uint16_t heading{};
  vec3<int32_t> position{};
  vec2<int32_t> last_position{};
  bool selected{};
  std::optional<size_t> actor;
  std::optional<vec3<int32_t>> destination;
  uint8_t destination_colour{};
  bool blink{};
};
struct tactical_map_state {
  vec3<int32_t> camera{};
  rectangle mission_bounds;
  uint32_t ticks{};
  bool failed{};
  std::vector<tactical_contact> contacts;
};
struct tactical_hit { rectangle bounds; size_t contact; };

int32_t initial_tactical_height(rectangle const &mission_bounds, rectangle const &screen_bounds);
int32_t tactical_pan_step(int32_t height);
vec2<int32_t> tactical_project(vec3<int32_t> const &point, vec3<int32_t> const &camera, rectangle const &bounds);
/// Original north-up lower-panel map: fixed grid, cell-1 scenery and ICONS.DBA.
class tactical_map {
  framebuffer &target;
  scenery_world const &world;
  palette colours;
  model_renderer models;
  std::vector<bitmap> icons;
public:
  tactical_map(resource_store const &resources, framebuffer &target, scenery_world const &world);
  std::vector<tactical_hit> draw(rectangle const &bounds, tactical_map_state const &state);
};
} // namespace earthsiege
