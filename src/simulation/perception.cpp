#include "perception.h"
#include <stdexcept>
#include "simulation/fixed_math.h"

namespace earthsiege {

actor_awareness::actor_awareness(size_t const actor_count) : known(actor_count), visible(actor_count) {}

void share_contact(std::span<perception_actor const> const actors, size_t const source_index, size_t const target_index, mission_triggers &triggers, std::span<int16_t> const counters) {
  if(source_index >= actors.size() || target_index >= actors.size()) throw std::out_of_range{"contact sharing actor reference"};
  auto const &source{actors[source_index]}; auto const &target{actors[target_index]};
  if(source.side == target.side) return;
  for(auto const &observer : actors) {
    if(observer.waiting || observer.side != source.side) continue;
    vec3<int32_t> delta;
    for(unsigned int axis{0}; axis != 3; ++axis) delta[axis] = wrap_long(static_cast<int64_t>(target.position[axis]) - observer.position[axis]);
    auto const distance{fast_distance(delta)};
    if(distance >= 120000) continue;
    observer.awareness.known.at(target_index) = 1;
    if(distance < 50000) {
      target.awareness.alerted = true;
      if(observer.trigger && !observer.awareness.event_busy) triggers.activate(*observer.trigger, counters);
    }
  }
}

bool cached_visibility(actor_awareness &observer, actor_awareness const &target, size_t const target_index, random_state &random, std::function<bool()> const &line_of_sight) {
  auto &visible{observer.visible.at(target_index)};
  if(target.visibility_remaining == 0) {
    observer.visibility_remaining = static_cast<int16_t>(5000 + random.bounded(1000));
    visible = line_of_sight();
  }
  return visible != 0;
}

} // namespace earthsiege
