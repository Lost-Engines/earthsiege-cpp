#pragma once
#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include "resources/mission_script.h"
#include "simulation/actor_contact.h"

namespace earthsiege {

struct movement_request {
  int16_t flags{0};
  int16_t speed{0};
  int16_t turn{0};
};

movement_request follow_route(vec3<int32_t> const &position, uint16_t heading, int16_t requested_speed, mission_route const &route, std::span<vec3<int32_t> const> points, int16_t &cursor);
movement_request follow_formation(vec3<int32_t> const &position, uint16_t heading, vec3<int32_t> const &leader_position, uint16_t leader_heading, int16_t leader_speed, vec3<int32_t> const &offset);

enum class avoidance_probe {
  scenery,
  ground_objects,
};

struct avoidance_state {
  vec3<int32_t> position;
  uint16_t heading{0};
  int16_t current_speed{0};
  int32_t collision_countdown{0};
  int16_t collision_bias{0};
  bool forward_collision{false};
};

using avoidance_query = std::function<std::optional<int32_t>(avoidance_probe, vec3<int32_t> const &, vec3<int32_t> const &)>;
void avoid_obstacles(movement_request &control, avoidance_state const &state, std::span<contact_actor const> actors, avoidance_query const &query);

} // namespace earthsiege
