#include "player_targeting.h"
#include <stdexcept>
#include "simulation/model_math.h"

namespace earthsiege {
bool can_target_from_cockpit(cockpit_observer const &observer, cockpit_target const &target) {
  if(target.dead || target.disabled || target.side == observer.side) return false;
  vec3<int32_t> delta;
  for(unsigned int axis{0}; axis != 3; ++axis) delta[axis] = wrap_long(static_cast<int64_t>(target.position[axis]) - observer.position[axis]);
  auto const distance{fast_distance(delta)};
  if(!((target.seen && distance < observer.visible_range) || (target.known && distance < observer.contact_range))) return false;
  auto const error{wrap_word(bearing_angle(observer.position, target.position) - observer.heading + observer.torso)};
  return error >= -8999 && error <= 8999;
}
std::optional<size_t> cycle_cockpit_target(std::span<cockpit_target const> const actors, cockpit_observer const &observer, std::optional<size_t> const previous) {
  if(actors.empty()) return std::nullopt;
  if(previous && *previous >= actors.size()) throw std::out_of_range{"cockpit target ordinal"};
  auto const first{previous ? (*previous + 1) % actors.size() : 0};
  for(size_t step{0}; step != actors.size(); ++step) {
    auto const candidate{(first + step) % actors.size()};
    if(previous == candidate) break;
    if(can_target_from_cockpit(observer, actors[candidate])) return candidate;
  }
  return previous;
}
} // namespace earthsiege
