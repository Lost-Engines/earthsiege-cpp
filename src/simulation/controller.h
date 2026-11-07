#pragma once
#include <functional>
#include <optional>
#include <string_view>
#include "simulation/frame_time.h"

namespace earthsiege {

enum class behavior : uint8_t {
  herc_deciding, player, attacking, flanking, facing_off, attacking_base,
  attacking_flyer, patrolling, travelling, bulldog_travel, search_destroy,
  sleeping, skirting, guarding, driving_off_enemy, ramming, fleeing, in_limbo,
  herc_dead, disabled,
  flyer_deciding, flyer_attacking, flyer_patrolling, flyer_search_destroy,
  flyer_sleeping, flyer_scouting, flyer_dead,
};

struct behavior_definition {
  std::string_view name;
  int32_t reconsider_period;
  uint16_t properties;                                                      // original sixteen Boolean properties, packed in stored order
  std::optional<int16_t> status;                                             // native HERC HUD status index; flyers have no entry
  bool holds_countdown() const;
  bool attacking() const;
};
behavior_definition const &describe(behavior type, bool expansion = false);

struct controller_state {
  std::optional<behavior> type;
  int32_t reconsider_remaining{0};
  uint32_t ticks{0};
  bool updated{false};
  bool first_entry{false};
  int16_t target_search_remaining{0};
};

void enter_behavior(controller_state &state, behavior type, uint16_t &world_stagger);
void finish_controller_update(controller_state &state);
behavior initial_herc_behavior(bool player, int16_t order, int16_t torso_yaw_limit);
behavior initial_flyer_behavior(int16_t order);
int16_t herc_status(behavior type, bool disabled, bool dead, int16_t command, bool expansion = false);

struct controller_callbacks {
  std::function<void(behavior)> reconsider;
  std::function<void(behavior)> advance;
  std::function<bool(behavior)> decide;
};
/// Advance with the previous controls, then decide the next controls. Actor
/// vtable +0x18 calls behavior +0x1c, and vtable +0x1c calls behavior +0x18.
/// Callbacks may enter a new behavior; later phases observe that new state.
void dispatch_controller(controller_state &state, bool &skip_decision, frame_ratio ratio, controller_callbacks const &callbacks);

} // namespace earthsiege
