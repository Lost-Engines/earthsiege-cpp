#pragma once
#include <cstddef>
#include <cstdint>
#include <optional>
#include "vectorstorm/vector/vector3.h"

namespace earthsiege {

struct herc_impact_state {
  bool detectable{false};
  bool shield_warning{false};
  int16_t ecm_remaining{0};
  int32_t damage_received{0};
};

struct impact_conditions {
  bool player{false};
  uint8_t attacker_side{0};
  int16_t difficulty{0};
  bool difficulty_override{false};
};

struct body_damage_request {
  size_t section{0};
  int16_t direct{0};
  int16_t radial{0};
};

struct herc_interception {
  /// Original actor-method depth, including its one-unit adjustment; world dispatch retains it
  int32_t distance{0};
  vec3<int32_t> position;
  std::optional<body_damage_request> body;
};

} // namespace earthsiege
