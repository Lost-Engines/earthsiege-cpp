#include "perception.h"
#include <stdexcept>
#include <algorithm>
#include "simulation/model_math.h"
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

bool within_actor_view(uint16_t const heading, int16_t const bearing) {
  auto const difference{static_cast<int32_t>(bearing)-wrap_word(heading)};
  return difference >= -14336 && difference < 14336;
}

void advance_perception(std::span<perception_actor const> const actors, int16_t const ratio, int32_t const detection_range, random_state &random, mission_triggers &triggers, std::span<int16_t> const counters, perception_callbacks const &callbacks) {
  if(!callbacks.line_of_sight || !callbacks.orientation_offset || !callbacks.within_view) throw std::invalid_argument{"perception callbacks are incomplete"};
  auto const delta{[](vec3<int32_t> const &a, vec3<int32_t> const &b){ return vec3<int32_t>{wrap_long(static_cast<int64_t>(a.x)-b.x),wrap_long(static_cast<int64_t>(a.y)-b.y),wrap_long(static_cast<int64_t>(a.z)-b.z)}; }};
  auto const tick{[&](int16_t &clock){ clock = std::max<int16_t>(0,wrap_word(clock-ratio)); return clock == 0; }};
  auto const visible{[&](size_t observer, size_t target){ return cached_visibility(actors[observer].awareness,actors[target].awareness,target,random,[&]{return callbacks.line_of_sight(observer,target);}); }};
  for(size_t index{0}; index != actors.size(); ++index) {
    auto const &observer{actors[index]}; if(observer.waiting) continue;
    auto &knowledge{observer.awareness}; tick(knowledge.visibility_remaining);
    if(!tick(knowledge.refresh_remaining)) continue;
    knowledge.refresh_remaining = static_cast<int16_t>(10000+random.bounded(1000));
    knowledge.seen = false;
    for(size_t other{0}; other != actors.size(); ++other) {
      auto const &target{actors[other]};
      if(target.waiting || target.side == observer.side || !knowledge.known.at(other)) continue;
      auto const separation{delta(target.position,observer.position)};
      if(fast_distance_2d({separation.x,separation.y}) > 120000 || !visible(index,other)) {
        knowledge.known[other] = 0; target.awareness.known.at(index) = 0;
      }
    }
  }
  auto const scan{[&](size_t const index){
    auto const &observer{actors[index]}; auto &knowledge{observer.awareness};
    for(size_t other{0}; other != actors.size(); ++other) {
      auto const &target{actors[other]}; auto &contacts{target.awareness};
      if(target.waiting || target.dead || target.disabled || target.side != 1) continue;
      auto const distance{fast_distance(delta(target.position,observer.position))};
      if((!knowledge.seen || !contacts.seen) && (observer.detectable || target.detectable) && distance < 100000 && visible(index,other)) {
        if(callbacks.noticed) callbacks.noticed(index,other);
        knowledge.seen = contacts.seen = true;
      }
      if(distance >= detection_range) continue;
      if(!knowledge.known.at(other) || !contacts.known.at(index)) {
        auto const bearing{bearing_angle(observer.position,target.position)};
        if(callbacks.within_view(index,wrap_word(bearing+callbacks.orientation_offset(index))) && visible(index,other)) {
          if(callbacks.noticed) callbacks.noticed(index,other);
          if(observer.player) knowledge.known[other] = 1;
          else share_contact(actors,index,other,triggers,counters);
        }
        if(callbacks.within_view(other,wrap_word(bearing+callbacks.orientation_offset(other)-32768)) && visible(index,other)) {
          // Reverse recognition still reuses the observer's visibility query.
          share_contact(actors,other,index,triggers,counters);
        }
      } else if(distance < 50000) {
        contacts.alerted = true;
        if(observer.trigger && !knowledge.event_busy) triggers.activate(*observer.trigger,counters);
        knowledge.alerted = true;
        if(target.trigger && !contacts.event_busy) triggers.activate(*target.trigger,counters);
      }
    }
  }};
  std::optional<size_t> player;
  for(size_t index{0}; index != actors.size(); ++index) {
    auto const &actor{actors[index]};
    if(actor.waiting || actor.disabled || actor.dead || actor.side != 0) continue;
    if(actor.player) player = index; else scan(index);
  }
  if(player) scan(*player);
  for(auto const &actor : actors) actor.awareness.event_busy = false;
}

} // namespace earthsiege
