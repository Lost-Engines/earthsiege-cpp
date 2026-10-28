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
  int16_t refresh_remaining{0};
  bool seen{false};
  explicit actor_awareness(size_t actor_count);
};

struct perception_actor {
  vec3<int32_t> position;
  uint8_t side{0};
  bool waiting{false};
  std::optional<size_t> trigger;
  actor_awareness &awareness;
  bool dead{false};
  bool disabled{false};
  bool player{false};
  bool detectable{false};
};

void share_contact(std::span<perception_actor const> actors, size_t source, size_t target, mission_triggers &triggers, std::span<int16_t> counters);
/// Refresh is gated by the target's clock but resets the observer's clock in the original.
bool cached_visibility(actor_awareness &observer, actor_awareness const &target, size_t target_index, random_state &random, std::function<bool()> const &line_of_sight);

/// Original signed-angle visibility test, including its non-wrapping subtraction.
bool within_actor_view(uint16_t heading, int16_t bearing);

struct perception_callbacks {
  std::function<bool(size_t, size_t)> line_of_sight;
  std::function<int16_t(size_t)> orientation_offset;
  std::function<bool(size_t, int16_t)> within_view;
  std::function<void(size_t, size_t)> noticed;
};
/// Original world pass: expire contacts, scan side zero with the player last,
/// then clear each actor's event-busy latch. Detection range is 30000 or 60000.
void advance_perception(std::span<perception_actor const> actors, int16_t ratio, int32_t detection_range, random_state &random, mission_triggers &triggers, std::span<int16_t> counters, perception_callbacks const &callbacks);

} // namespace earthsiege
