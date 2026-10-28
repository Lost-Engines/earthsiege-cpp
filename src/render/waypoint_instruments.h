#pragma once
#include <optional>
#include "vectorstorm/vector/vector3.h"
#include "resources/cockpit.h"

namespace earthsiege {
struct hud_waypoint_state {
  vec3<int32_t> position;
  int16_t number;
};
class waypoint_instruments {
  rectangle bounds;
  font face;
public:
  waypoint_instruments(resource_store const &resources, cockpit_layout const &layout);
  void draw(framebuffer &target, vec3<int32_t> const &position, uint16_t heading, uint16_t tape_heading,
            std::optional<hud_waypoint_state> const &waypoint, int offset_x = 0, int offset_y = 0) const;
};
} // namespace earthsiege
