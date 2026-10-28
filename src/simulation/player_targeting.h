#pragma once
#include <optional>
#include <span>
#include "simulation/fixed_math.h"

namespace earthsiege {
inline int32_t constexpr cockpit_visible_target_range{120000};
struct cockpit_target {
  vec3<int32_t> position{};
  uint8_t side{0};
  bool seen{false};
  bool known{false};
  bool dead{false};
  bool disabled{false};
};
struct cockpit_observer {
  vec3<int32_t> position{};
  uint16_t heading{0};
  int16_t torso{0};
  uint8_t side{0};
  int32_t contact_range{60000};
  int32_t visible_range{cockpit_visible_target_range};
};
bool can_target_from_cockpit(cockpit_observer const &observer, cockpit_target const &target);
/// Enter/Tab scans original actor order inside the ±8999 forward cone.
/// With no eligible replacement, retain the existing selection, even if stale.
std::optional<size_t> cycle_cockpit_target(std::span<cockpit_target const> actors, cockpit_observer const &observer, std::optional<size_t> previous);
} // namespace earthsiege
