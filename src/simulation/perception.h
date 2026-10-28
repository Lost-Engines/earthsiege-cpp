#pragma once
#include <functional>
#include <optional>
#include <span>
#include <vector>
#include "simulation/mission_triggers.h"
#include "simulation/random_state.h"

namespace earthsiege {

/// One actor's contact knowledge and cached line of sight, indexed by world ordinal.
struct actor_awareness {
  std::vector<uint8_t> known;
  std::vector<uint8_t> visible;
  int16_t visibility_remaining{0};
  bool alerted{false};
  bool event_busy{false};
  explicit actor_awareness(size_t actor_count);
};

struct perception_actor {
  vec3<int32_t> position;
  uint8_t side{0};
  bool waiting{false};
  std::optional<size_t> trigger;
  actor_awareness &awareness;
};

void share_contact(std::span<perception_actor const> actors, size_t source, size_t target, mission_triggers &triggers, std::span<int16_t> counters);
/// Refresh is gated by the target's clock but resets the observer's clock in the original.
bool cached_visibility(actor_awareness &observer, actor_awareness const &target, size_t target_index, random_state &random, std::function<bool()> const &line_of_sight);

} // namespace earthsiege
