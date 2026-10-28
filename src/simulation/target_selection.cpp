#include "target_selection.h"
#include <algorithm>
#include <array>
#include <cstdlib>
#include <stdexcept>
#include "simulation/fixed_math.h"
#include "simulation/model_math.h"

namespace earthsiege {
namespace {

int32_t distance_between(target_candidate const &first, target_candidate const &second) {
  vec3<int32_t> delta;
  for(unsigned int axis{0}; axis != 3; ++axis) delta[axis] = wrap_long(static_cast<int64_t>(first.position[axis]) - second.position[axis]);
  return fast_distance(delta);
}

int saturated_magnitude(int16_t const value) { return std::abs(std::max(-32767, static_cast<int>(value))); }
int32_t q10(int32_t const value, int16_t const factor) { return wrap_long(static_cast<int64_t>(value) * factor + 512) >> 10; }

size_t assess_strength(target_candidate const &observer, target_candidate const &target, random_state &random) {
  if(!observer.compares_combat_strength) return 1;
  if(target.kind != combat_actor_kind::herc) return 0;
  // The original masks by 1000, rather than taking a remainder or uniform range.
  auto const first{wrap_word(observer.strength + (random.next() & 1000))};
  auto const second{wrap_word(target.strength + (random.next() & 1000))};
  return saturated_magnitude(wrap_word(first - second)) < 400 ? 1 : first < second ? 2 : 0;
}

} // anonymous namespace

bool recognizes(target_candidate const &observer, target_candidate const &target, size_t const target_index) {
  if(target_index >= observer.known.size()) throw std::out_of_range{"target ordinal exceeds awareness table"};
  return observer.known[target_index] != 0 || (target.seen && distance_between(observer, target) < 1000000);
}

std::optional<size_t> select_target(std::span<target_candidate const> const actors, size_t const observer_index, target_selection_flags const flags, target_objective const &objective, int16_t const mission_kind, std::optional<size_t> const hud_target, random_state &random) {
  if(observer_index >= actors.size() || objective.leader >= actors.size()) throw std::out_of_range{"target selection actor reference"};
  auto const &observer{actors[observer_index]};
  auto const &leader{actors[objective.leader]};
  auto const has{[flags](target_selection_flags const flag){ return (static_cast<uint8_t>(flags) & static_cast<uint8_t>(flag)) != 0; }};
  std::array<int16_t, 3> constexpr retaliation{1500, 2000, 2500}, passive{600, 400, 200}, engaged_elsewhere{700, 850, 1000}, kind_weights{2000, 500, 200};
  int best_priority{0};
  int16_t best_score{0};
  std::optional<size_t> selected;
  for(size_t index{0}; index != actors.size(); ++index) {
    auto const &target{actors[index]};
    if(target.side == observer.side || target.dead || target.removed || !recognizes(observer, target, index)) continue;
    if(has(target_selection_flags::different_actor_kind) && target.kind == observer.kind) continue;
    if(leader.player && mission_kind == 3 && objective.representative == index && !leader.objective_event) continue;
    if(observer.ignore_hud_target && hud_target == index) continue;
    bool const designated{objective.order == 1 && std::ranges::find(objective.actors, index) != objective.actors.end()};
    bool const active{!target.disabled && !target.unavailable};
    int const priority{2 * static_cast<int>(designated || !has(target_selection_flags::prefer_objective)) + static_cast<int>(active)};
    if(target.disabled && !(designated && observer.side != 0)) continue;
    if(priority < best_priority) continue;
    auto const distance{distance_between(observer, target)};
    auto const error{has(target_selection_flags::ignore_bearing) ? 0 : saturated_magnitude(wrap_word(static_cast<int32_t>(bearing_angle(observer.position, target.position)) - observer.heading))};
    auto const angular{static_cast<int16_t>(4096 - (error >> 3))};
    int32_t score{q10(100, angular)};
    if(distance < 25000) score = std::max(score, angular / std::max(1, distance >> 11));
    auto const assessment{assess_strength(observer, target, random)};
    if(!has(target_selection_flags::ignore_retaliation) && target.target == observer_index) score = wrap_word(q10(score, retaliation[assessment]));
    if(target.controller_attacking && !*target.controller_attacking) score = wrap_word(q10(score, passive[assessment]));
    if(target.target && target.target != observer_index) score = wrap_word(q10(score, engaged_elsewhere[assessment]));
    score = wrap_word(q10(score, kind_weights.at(static_cast<size_t>(target.kind))));
    auto const attackers{wrap_word(target.attackers - static_cast<int>(observer.target == index))};
    if(!has(target_selection_flags::ignore_other_attackers) && attackers != 0) {
      if(attackers == -1) throw std::runtime_error{"inconsistent target attacker count would divide by zero"};
      score = wrap_word(score / (attackers + 1));
    }
    if(score > best_score || priority > best_priority) {
      selected = index; best_score = static_cast<int16_t>(score); best_priority = priority;
    }
  }
  return selected;
}

} // namespace earthsiege
