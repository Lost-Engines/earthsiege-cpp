#include "component_damage.h"
#include <algorithm>
#include <stdexcept>
#include <utility>
#include "simulation/fixed_math.h"

namespace earthsiege {

bool accumulate_damage(int16_t &amount, int16_t const capacity, int16_t &current) {
  if(current >= 0) {
    current = wrap_word(current + amount);
    if(current >= capacity) {
      amount = wrap_word(current - capacity);
      current = -1;
    }
  }
  return current < 0;
}

component_damage::component_damage(damage_definition const &source, damage_state initial) : definition{source}, current{std::move(initial)} {
  if(current.internal.size() != definition.hit_points.size() || current.armour.size() != definition.components.size() || current.present.size() != definition.components.size()) throw std::invalid_argument{"damage state does not match its definition"};
}

damage_state const &component_damage::state() const { return current; }

bool component_damage::distribute(size_t const section, int16_t amount, component_choice const &choose) {
  auto const &dependencies{definition.components.at(section).dependents};
  while(true) {
    int16_t total{0};
    for(auto const &entry : dependencies) if(current.internal.at(static_cast<size_t>(entry.component)) >= 0) total = wrap_word(total + entry.weight);
    if(total == 0) return true;
    auto choice{choose(total)};
    if(choice < 0 || choice >= total) throw std::out_of_range{"component random choice"};
    bool selected{false};
    for(auto const &entry : dependencies) {
      auto const slot{static_cast<size_t>(entry.component)};
      if(current.internal.at(slot) < 0) continue;
      choice = wrap_word(choice - entry.weight);
      if(choice >= 0) continue;
      if(!accumulate_damage(amount, definition.hit_points.at(slot), current.internal[slot])) return false;
      selected = true;
      break;
    }
    if(!selected) throw std::runtime_error{"component weights did not select an internal system"};
  }
}

void component_damage::remove(size_t const section, component_choice const &choose, section_detachment const &detach, std::vector<size_t> &pending, std::vector<size_t> &removed) {
  auto const &piece{definition.components.at(section)};
  if(current.present.at(section) == 0) return;
  removed.push_back(section);
  current.armour[section] = -1;
  current.present[section] = 0;
  int16_t constexpr detachment_damage{32000};
  distribute(section, detachment_damage, choose);
  if(piece.cell_sequence >= 0) current.cells.at(static_cast<size_t>(piece.cell_sequence)) = 2;
  if(detach) detach(section);                                                  // effect RNG consumption precedes the next dependent-section removal
  for(size_t i{0}; i != definition.components.size(); ++i) if(current.present[i] && definition.components[i].parent == static_cast<int>(section)) pending.push_back(i);
}

std::vector<size_t> component_damage::hit(size_t const section, int16_t amount, int16_t const legs, component_choice const &choose, section_detachment const &detach) {
  std::vector<size_t> removed;
  if(current.present.at(section) == 0) return removed;
  auto const &piece{definition.components.at(section)};
  if(accumulate_damage(amount, piece.armour, current.armour[section])) {
    if(!distribute(section, amount, choose)) {
      if(piece.cell_sequence >= 0) current.cells.at(static_cast<size_t>(piece.cell_sequence)) = 1;
    } else if(piece.flags & std::to_underlying(damage_section_flag::detachable)) {
      std::vector<size_t> pending;
      remove(section, choose, detach, pending, removed);
      while(!pending.empty()) {
        auto const next{pending.back()};
        pending.pop_back();
        remove(next, choose, detach, pending, removed);
      }
    }
  }
  update_consequences(legs);
  return removed;
}

void component_damage::update_consequences(int16_t const legs) {
  auto &flags{current.consequences};
  auto const fraction{[&](size_t const component){
    auto const value{current.internal.at(component)};
    if(value < 0) return 256;
    auto const capacity{definition.hit_points.at(component)};
    if(capacity == 0) throw std::domain_error{"zero component damage capacity"};
    return value * 256 / capacity;
  }};
  if(!flags.disabled && legs != 0) {
    std::vector<int> grades{fraction(0), fraction(1)};
    if(legs > 2) { grades.push_back(fraction(10)); grades.push_back(fraction(11)); }
    auto left{grades[0]}, right{grades[1]};
    if(legs == 4) { left = (left + grades[2]) >> 1; right = (right + grades[3]) >> 1; }
    if(std::ranges::count(grades, 256) >= legs / 2) flags.disabled = true;
    else if(left >= 193 || right >= 193) flags.movement_heavy = true;
    else if(left > 128 || right > 128) flags.movement_light = true;
  }
  auto const destroyed{[&](size_t const section){
    return current.armour.at(section) < 0 && std::ranges::all_of(definition.components.at(section).dependents, [&](damage_dependency const &entry){ return current.internal.at(static_cast<size_t>(entry.component)) < 0; });
  }};
  if(!flags.dead && (destroyed(0) || destroyed(1) || fraction(9) == 256)) flags.dead = true;
  if(!flags.component5_light && !flags.component5_heavy) {
    int const damage{fraction(5)};
    if(damage >= 193) flags.component5_heavy = true;
    else if(damage > 128) flags.component5_light = true;
  }
}

} // namespace earthsiege
