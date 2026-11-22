#pragma once
#include <span>
#include <vector>
#include "simulation/controller.h"
#include "simulation/fixed_math.h"

namespace earthsiege {
struct wingmate_order_state {
  vec3<int32_t> position{};
  behavior type{behavior::patrolling};
  bool unavailable{false};
  bool disabled{false};
  bool dead{false};
  bool radar{false};
  bool radar_requested{false};
  bool ignore_target{false};
  int16_t command{0};
  std::optional<size_t> designated;
  std::optional<size_t> selected;
  int16_t stop_status{0};
  vec2<int32_t> command_position{};
  int32_t incoming_damage{0};
  int32_t reconsider_remaining{0};
  bool herc{true};
  bool expansion{false};
  bool weapons_free{false};
  std::optional<size_t> followed;
};
struct wingmate_order {
  uint16_t command{0};
  size_t issuer{0};
  vec3<int32_t> position{};
  std::optional<size_t> target;
};
struct wingmate_order_callbacks {
  std::function<std::optional<size_t>(bool)> select;
  std::function<void(size_t)> attack;
  std::function<void()> resume;
  std::function<void(behavior)> enter;
  std::function<void(uint16_t)> acknowledge;
};
/// Command receiver 0x1a1cc. Callbacks observe preceding field writes and may
/// update the actor after strategy entry. Previous reply: 0 none, 1 refused, 2 accepted.
bool receive_wingmate_order(std::span<wingmate_order_state> actors, size_t actor,
  wingmate_order const &order, uint16_t previous_reply, size_t player, size_t leader,
  int16_t group_order, std::optional<size_t> hud_target, wingmate_order_callbacks const &callbacks, std::optional<size_t> guard_objective = std::nullopt);
/// Group dispatcher 0x1c950: recomputes priority after each member's response.
/// The original leaves priority uninitialized for orders without a priority
/// rule (notably FORM UP). Zero represents the ordinary neutral-priority path.
bool dispatch_wingmate_order(std::span<wingmate_order_state const> actors,
  std::span<size_t const> group, wingmate_order const &order,
  std::function<bool(size_t, uint16_t)> const &deliver, int16_t fallback_priority = 0);
} // namespace earthsiege
