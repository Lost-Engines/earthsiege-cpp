#pragma once
#include "simulation/actor_placement.h"
#include "simulation/engagement.h"
#include "simulation/perception.h"

namespace earthsiege {

struct mission_actor_activity {
  actor_kind kind;
  size_t group;
  size_t member;
  bool player;
  bool persistent_signature;
  std::optional<size_t> contact_trigger;
  std::optional<size_t> hit_trigger;
  int16_t counter{-1};
  int16_t counter_operation{-1};
  actor_awareness awareness;
};

struct mission_group_activity {
  actor_kind kind;
  int16_t order;
  int16_t formation;
  int16_t route{-1};
  int16_t route_cursor{0};
  uint8_t side;
  std::optional<size_t> activation_trigger;
  std::vector<size_t> members;
  std::vector<size_t> objectives;
  std::optional<size_t> representative;
  std::optional<size_t> objective_group{};
  int16_t counter{-1};
  int16_t counter_operation{-1};
};

struct observed_actor {
  vec3<int32_t> position;
  uint16_t heading{0};
  int16_t view_offset{0};
  int16_t strength{0};
  bool dead{false};
  bool disabled{false};
  bool detectable{false};
  bool removed{false};
  bool unavailable{false};
  bool objective_event{false};
  bool ignore_hud_target{false};
};

/// Mission-owned identity, groups, activation latches, contacts and target links.
/// Positions and damage remain owned by the corresponding live actors.
class mission_activity {
  mission_header settings;
  mission_counters counters;
  mission_triggers triggers;
  std::vector<mission_actor_activity> actors;
  std::vector<mission_group_activity> groups;
  std::vector<engagement_state> controllers;
  std::vector<target_relationship> relationships;
  uint16_t stagger{0};
  size_t player{0};
  std::vector<perception_actor> perception_views(std::span<observed_actor const> observations);

public:
  mission_activity(mission_script const &script, mission_placement const &placement, mission_counters initial = {});
  mission_actor_activity const &actor(size_t index) const;
  mission_actor_activity &actor(size_t index);
  mission_group_activity const &group(size_t index) const;
  mission_group_activity &group(size_t index);
  std::span<mission_group_activity const> all_groups() const;
  std::span<engagement_state> decisions();
  std::span<engagement_state const> decisions() const;
  std::span<target_relationship> targets();
  uint16_t &entry_stagger();
  size_t size() const;
  bool waiting(size_t index) const;
  mission_counters const &variables() const;
  bool triggered(size_t index) const;
  void activate(size_t index);
  void notify_destroyed(size_t index, std::span<observed_actor const> observations);
  target_objective objective(size_t index) const;
  void dispatch_groups(std::function<void(size_t)> const &dispatch);
  void scan_areas(std::span<observed_actor const> observations);
  void perceive(std::span<observed_actor const> observations, frame_ratio ratio, random_state &random, std::function<bool(size_t,size_t)> const &line_of_sight, std::function<void(size_t,size_t)> const &noticed = {});
  void share_contact(size_t observer, size_t target, std::span<observed_actor const> observations);
  bool eligible(std::span<observed_actor const> observations, size_t observer, size_t candidate) const;
  std::optional<size_t> select(std::span<observed_actor const> observations, size_t observer, target_selection_flags flags, std::optional<size_t> hud_target, random_state &random) const;
};
} // namespace earthsiege
