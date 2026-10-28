#pragma once
#include <span>
#include "simulation/controller.h"
#include "simulation/target_selection.h"

namespace earthsiege {

struct target_relationship {
  std::optional<size_t> selected;
  int16_t attackers{0};
  bool changed{false};
};
void assign_target(std::span<target_relationship> relationships, size_t observer, std::optional<size_t> selected);

enum class engagement_mode { herc_search, herc_patrol, flyer_search, flyer_patrol };

struct engagement_state {
  controller_state controller;
  int16_t command{0};
  int16_t aimed_component{-1};
  bool unavailable{false};
  bool disabled{false};
  bool dead{false};
  bool attack_requested{false};                                              // original +0xa6 before HERC strategy entry
};

struct engagement_callbacks {
  std::function<void()> navigate;
  std::function<std::optional<size_t>(target_selection_flags)> select;
  std::function<void()> attack;
  std::function<void(behavior)> enter{};
};

void patrol_engagement(engagement_mode mode, engagement_state &state, size_t actor, std::span<target_relationship> relationships, target_objective const &objective, frame_ratio ratio, uint16_t &world_stagger, engagement_callbacks const &callbacks);
void begin_flyer_attack(std::span<engagement_state> actors, std::span<target_relationship> relationships, size_t actor, std::span<size_t const> group, uint16_t &world_stagger);

} // namespace earthsiege
