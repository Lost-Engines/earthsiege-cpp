#include "controller.h"
#include <algorithm>
#include <array>
#include <stdexcept>
#include "simulation/fixed_math.h"

namespace earthsiege {

bool behavior_definition::holds_countdown() const { return (properties & 1U) != 0; }
bool behavior_definition::attacking() const { return (properties & 2U) != 0; }

behavior_definition const &describe(behavior const type, bool const expansion) {
  static std::array<behavior_definition, 27> constexpr definitions{{
    {"deciding", 0, 0, 3}, {"player", 10, 1, 3}, {"attacking", 50000, 2, 0},
    {"flanking", 50000, 2, 0}, {"facing off", 50000, 2, 0}, {"attacking base", 50000, 2, 0},
    {"attacking flyer", 50000, 2, 0}, {"patrolling", 10, 1, 3}, {"travelling", 10, 1, 3},
    {"bulldog travel", 10, 9, 3}, {"search/destroy", 10, 1, 3}, {"sleeping", 10, 9, 3},
    {"skirting", 10, 1, 0}, {"guarding", 10, 5, 3}, {"driving off en", 50000, 6, 0},
    {"ramming", 10, 9, 0}, {"fleeing", 30000, 18, 5}, {"in limbo", 10, 33, 6},
    {"dead", 0, 33, 6}, {"disabled", 0, 33, 7},
    {"deciding", 0, 0, std::nullopt}, {"attacking", 50000, 1, std::nullopt},
    {"patrolling", 5000, 0, std::nullopt}, {"search and destroy", 5000, 0, std::nullopt},
    {"sleeping", 500, 1, std::nullopt}, {"scouting", 5000, 0, std::nullopt}, {"dead", 500, 1, std::nullopt},
  }};
  // Expansion table b693c keeps a HERC attacking while it skirts an obstacle.
  static constexpr behavior_definition expansion_skirting{"skirting", 10, 3, 0};
  if(expansion && type == behavior::skirting) return expansion_skirting;
  return definitions.at(static_cast<size_t>(type));
}

void enter_behavior(controller_state &state, behavior const type, uint16_t &world_stagger) {
  world_stagger = static_cast<uint16_t>(world_stagger + 13);
  state = {};
  state.type = type;
  state.reconsider_remaining = wrap_long(static_cast<int64_t>(describe(type).reconsider_period) + (world_stagger & 15));
  state.first_entry = true;
}

void finish_controller_update(controller_state &state) {
  if(state.first_entry) state.first_entry = false;
  else { state.updated = true; ++state.ticks; }
}

behavior initial_herc_behavior(bool const player, int16_t const order, int16_t const torso_yaw_limit) {
  if(player) return behavior::player;
  switch(order) {
    case 1: return behavior::search_destroy;
    case 3: return behavior::ramming;
    case 6: return behavior::guarding;
    case 7: case 8: return behavior::patrolling;
    case 10: return behavior::sleeping;
    case 11: return torso_yaw_limit > 32000 ? behavior::bulldog_travel : behavior::travelling;
    default: throw std::invalid_argument{"unsupported original HERC group order"};
  }
}

behavior initial_flyer_behavior(int16_t const order) {
  switch(order) {
    case 1: return behavior::flyer_search_destroy;
    case 6: case 10: return behavior::flyer_sleeping;
    case 7: case 11: return behavior::flyer_scouting;
    case 8: return behavior::flyer_patrolling;
    default: throw std::invalid_argument{"unsupported original flyer group order"};
  }
}

int16_t herc_status(behavior const type, bool const disabled, bool const dead, int16_t const command, bool const expansion) {
  auto const &definition{describe(type, expansion)};
  if(!definition.status) throw std::invalid_argument{"flyer controller has no HERC status"};
  if(!disabled && !dead && *definition.status != 5 && !definition.attacking()) {
    switch(command) {
      case 1: return 1;
      case 2: return 2;
      case 3: case 6: return 4;
      default: break;
    }
  }
  return *definition.status;
}

void dispatch_controller(controller_state &state, bool &skip_decision, frame_ratio const ratio, controller_callbacks const &callbacks) {
  if(!state.type) return;
  if(state.reconsider_remaining == 0 && callbacks.reconsider) callbacks.reconsider(*state.type);
  if(!describe(state.type.value()).holds_countdown()) state.reconsider_remaining = std::max(0, wrap_long(static_cast<int64_t>(state.reconsider_remaining) - ratio));
  if(callbacks.advance) callbacks.advance(*state.type);
  if(skip_decision) skip_decision = false;
  else if(callbacks.decide && callbacks.decide(*state.type)) state.reconsider_remaining = 0;
  finish_controller_update(state);
}

} // namespace earthsiege
