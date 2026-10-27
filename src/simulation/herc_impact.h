#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include "simulation/random_state.h"
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
  uint8_t side{1};
  bool invulnerable{false};
};

/// Effect emitters execute synchronously, including their shared random draws.
/// Explosion/debris callbacks are required; the remaining notifications are optional.
struct herc_damage_events {
  std::function<void(int16_t, vec3<int32_t> const &, random_state &)> explosion;
  std::function<void(int16_t, vec3<int32_t> const &, random_state &)> debris;
  std::function<void(int16_t, vec3<int16_t> const &)> attached_smoke;
  std::function<void(size_t)> weapon_ejected;
  std::function<void(int16_t, int16_t)> salvage;
  std::function<void()> shield_warning;
  std::function<void(size_t)> cockpit_damage;
  std::function<void(size_t, int16_t)> body_damage;
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
