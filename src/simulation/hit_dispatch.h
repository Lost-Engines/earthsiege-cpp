#pragma once
#include <span>
#include "simulation/projectile_instance.h"
#include "simulation/random_state.h"
#include "simulation/scenery_collision.h"

namespace earthsiege {

/// A frame view of the active actor list; hit callbacks update flags before reactions are selected
struct impact_actor {
  vec3<int32_t> position;
  size_t group{0};
  uint8_t side{0};
  bool waiting{false};
  bool dead{false};
  bool disabled{false};
  bool inactive{false};
  bool quiet{false};
  bool hit_enemy{false};
  std::optional<size_t> trigger;
  /// Retained distance from the actor method, including its original depth-plus-one adjustment
  impact_query hit;
};

enum class obstruction_kind { actor, scenery };
struct impact_obstruction {
  obstruction_kind kind{obstruction_kind::actor};
  size_t index{0};
  bool operator==(impact_obstruction const &) const = default;
};

struct impact_events {
  std::function<void(size_t)> trigger;
  std::function<void(size_t, size_t, int16_t)> react;
  std::function<void(size_t)> friendly_warning;
  std::function<void(vec3<int32_t> const &, int16_t)> effect;
  std::function<void(impact_obstruction)> obstructed;
};

struct shot_source {
  size_t actor{0};
  bool player{false};
  std::span<size_t const> group_members;
  std::optional<vec3<int32_t>> target;
};

struct impact_world {
  std::span<impact_actor> actors;
  std::span<scenery_collider const> scenery;
  random_state &random;
  impact_events events;
};

/// Ground, scenery, then active actors in stored order, applying damage immediately
std::optional<int32_t> dispatch_impact(impact_request request, shot_source const &source, impact_world &world);

} // namespace earthsiege
