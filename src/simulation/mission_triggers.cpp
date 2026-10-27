#include "mission_triggers.h"
#include <algorithm>
#include <cstdlib>
#include <stdexcept>
#include <utility>
#include "simulation/fixed_math.h"

namespace earthsiege {

bool spatial_area::contains(vec3<int32_t> const &position) const {
  if(rectangular) return position.x > std::min(first.x, second.x) && position.x < std::max(first.x, second.x)
    && position.y > std::min(first.y, second.y) && position.y < std::max(first.y, second.y);
  auto const x{std::abs(static_cast<int64_t>(wrap_long(static_cast<int64_t>(position.x) - first.x)))};
  auto const y{std::abs(static_cast<int64_t>(wrap_long(static_cast<int64_t>(position.y) - first.y)))};
  return std::max(x, y) + (std::min(x, y) >> 1) < radius;
}

mission_triggers::mission_triggers(mission_script const &script) {
  for(auto const &source : script.areas) {
    spatial_area area{.rectangular{source.kind == 0}, .first{vec3<int32_t>{script.points.at(static_cast<size_t>(source.first_point))}}, .second{0, 0, 0}};
    if(area.rectangular) area.second = script.points.at(static_cast<size_t>(source.second_point_or_radius));
    else area.radius = source.second_point_or_radius * 10;
    areas.push_back(area);
  }
  for(auto const &source : script.triggers) {
    trigger entry{.audience{static_cast<trigger_audience>(source.mode)}, .areas{}, .counter{source.counter}, .operation{source.counter_operation}};
    for(auto const index : source.areas) {
      if(index < 0) break;
      if(static_cast<size_t>(index) >= areas.size()) throw std::out_of_range{"trigger area reference"};
      entry.areas.push_back(static_cast<size_t>(index));
    }
    triggers.push_back(std::move(entry));
  }
}

bool mission_triggers::latched(size_t const index) const { return triggers.at(index).latched; }

void mission_triggers::activate(size_t const index, std::span<int16_t> const counters) {
  auto &entry{triggers.at(index)};
  if(entry.latched) return;
  entry.latched = true;
  if(entry.counter < 0) return;
  if(static_cast<size_t>(entry.counter) >= counters.size()) throw std::out_of_range{"trigger counter index"};
  auto &counter{counters[static_cast<size_t>(entry.counter)]};
  if(entry.operation == 5) counter = 0;
  else if(entry.operation == 6) counter = wrap_word(counter + 1);
}

bool mission_triggers::enter(size_t const index, vec3<int32_t> const &position, std::span<int16_t> const counters) {
  auto const &entry{triggers.at(index)};
  if(entry.latched) return true;
  for(auto const area : entry.areas) if(areas[area].contains(position)) {
    activate(index, counters);
    return true;
  }
  return false;
}

void mission_triggers::scan(vec3<int32_t> const &player, size_t const player_group, std::span<trigger_group const> const groups, std::span<int16_t> const counters) {
  if(player_group >= groups.size()) throw std::out_of_range{"player trigger group"};
  for(size_t index{0}; index != triggers.size(); ++index) {
    auto const &entry{triggers[index]};
    if(entry.audience == trigger_audience::player) {
      enter(index, player, counters);
      continue;
    }
    for(size_t group_index{0}; group_index != groups.size(); ++group_index) {
      auto const &group{groups[group_index]};
      bool selected{false};
      switch(entry.audience) {
        case trigger_audience::player_group: selected = group_index == player_group; break;
        case trigger_audience::side_zero: selected = !group.waiting && group.side == 0; break;
        case trigger_audience::side_one: selected = !group.waiting && group.side == 1; break;
        case trigger_audience::hercs: selected = !group.waiting && group.kind == actor_kind::herc; break;
        case trigger_audience::flyers: selected = !group.waiting && group.kind == actor_kind::flyer; break;
        default: break;
      }
      if(!selected) continue;
      bool accepted{false};
      for(auto const &position : group.positions) if(enter(index, position, counters)) { accepted = true; break; }
      if(accepted) break;
    }
  }
}

} // namespace earthsiege
