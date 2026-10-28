#pragma once
#include "simulation/herc_tactics.h"
#include "simulation/engagement.h"

namespace earthsiege {

struct herc_travel_state { int32_t search_remaining{0}; std::optional<size_t> threat; };
struct herc_flee_state { int32_t turn_remaining{0}; bool turn_right{false}; std::optional<size_t> threat; };
struct herc_skirt_state {
  std::optional<behavior> previous;
  bool initialized{false};
  bool turn_left{false};
  int32_t check_remaining{0};
  int32_t distance{0};
};
struct herc_ram_state { int32_t search_remaining{0}; bool bursting{false}; int16_t burst_remaining{0}; };

/// World consumers are invoked in original order; position is read again after
/// movement preparation or ramming substeps, which can change the actor's heading.
struct herc_order_callbacks {
  std::function<herc_combat_position(size_t)> position;
  std::function<std::optional<size_t>(target_selection_flags)> select;
  std::function<void()> follow_route;
  std::function<void(movement_request const &)> move;
  std::function<void(size_t, int16_t)> aim;
  std::function<void()> centre_aim;
  std::function<bool()> path_blocked;
  std::function<void(behavior)> enter;
  std::function<void()> ram_advance;
};

void travel_with_herc(herc_travel_state &state, std::span<target_relationship> targets, size_t actor, bool persistent_signature, bool &detectable, frame_ratio ratio, herc_order_callbacks const &callbacks);
bool flee_with_herc(herc_flee_state &state, std::span<target_relationship> targets, size_t actor, int16_t &command, bool incapacitated, bool threat_incapacitated, frame_ratio ratio, random_state &random, herc_order_callbacks const &callbacks);
void skirt_with_herc(herc_skirt_state &state, size_t actor, vec3<int32_t> const &obstacle, bool &obstruction, frame_ratio ratio, herc_order_callbacks const &callbacks);
void ram_with_herc(herc_ram_state &state, std::span<target_relationship> targets, size_t actor, frame_ratio ratio, random_state &random, herc_order_callbacks const &callbacks);

} // namespace earthsiege
