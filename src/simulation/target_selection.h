#pragma once
#include <optional>
#include <span>
#include "resources/mission_script.h"
#include "simulation/random_state.h"

namespace earthsiege {

// Runtime combat categories differ from the mission file's HERC/flyer/base order.
enum class combat_actor_kind : int16_t { herc, base, flyer };
constexpr combat_actor_kind combat_kind(actor_kind kind) {
  return kind == actor_kind::herc ? combat_actor_kind::herc : kind == actor_kind::base ? combat_actor_kind::base : combat_actor_kind::flyer;
}

struct target_candidate {
  vec3<int32_t> position;
  uint16_t heading{0};
  combat_actor_kind kind{combat_actor_kind::herc};
  uint8_t side{0};
  int16_t strength{0};
  std::span<uint8_t const> known;
  bool seen{false};
  bool dead{false};
  bool removed{false};
  bool disabled{false};
  bool unavailable{false};                                                   // original actor +0x9f; producer retained separately
  bool objective_event{false};                                               // original actor +0x9a
  bool player{false};
  bool ignore_hud_target{false};
  std::optional<size_t> target;
  int16_t attackers{0};
  std::optional<bool> controller_attacking;
  bool compares_combat_strength{true};                                      // HERC override; ordinary actors always assess as comparable
};

struct target_objective {
  int16_t order{0};
  std::span<size_t const> actors;
  std::optional<size_t> representative;
  size_t leader{0};
};

enum class target_selection_flags : uint8_t {
  none = 0,
  prefer_objective = 1,
  ignore_retaliation = 2,
  ignore_other_attackers = 4,
  different_actor_kind = 16,
  ignore_bearing = 32,
};

enum class combat_balance : uint8_t { superior, comparable, inferior };
combat_balance compare_herc_strength(int16_t own_strength, int16_t target_strength, combat_actor_kind target_kind, random_state &random);

bool recognizes(target_candidate const &observer, target_candidate const &target, size_t target_index);
/// Retains native candidate order, priority classes and all strength-assessment RNG draws.
std::optional<size_t> select_target(std::span<target_candidate const> actors, size_t observer, target_selection_flags flags, target_objective const &objective, int16_t mission_kind, std::optional<size_t> hud_target, random_state &random, int16_t maximum_bearing = 0);

} // namespace earthsiege
