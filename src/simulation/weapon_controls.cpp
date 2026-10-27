#include "simulation/weapon_controls.h"
#include <algorithm>
#include <stdexcept>
#include <utility>
#include <vector>
#include "simulation/fixed_math.h"

namespace earthsiege {
namespace {

void countdown(int16_t &remaining, frame_ratio const ratio) {
  remaining = std::max<int16_t>(0, wrap_word(remaining - ratio));
}

void advance_cell(weapon_controls &state) {
  if(state.cell_count == 0) throw std::domain_error{"weapon animation has no cells"};
  state.cell = static_cast<uint16_t>((state.cell + 1U) % state.cell_count);
}

int16_t priority(capacitor const &state) {
  return state.fast ? 10000 : state.setting;
}

} // namespace

bool weapon_in_range(weapon_definition const &definition, int32_t const distance) {
  return definition.minimum_range < distance && distance < definition.range;
}

bool energy_ready(weapon_definition const &definition, capacitor const &state) {
  auto const low{definition.energy_thresholds[0]};
  auto const full{definition.energy_thresholds[1]};
  auto const required{low < full ? std::max(low, state.setting) : full};
  return !state.controls.disabled && state.controls.refire_remaining == 0 && state.charge >= required;
}

bool capacitor_charged(weapon_definition const &definition, capacitor const &state) {
  bool const recent{(state.controls.hud_changes & std::to_underlying(weapon_hud_change::fired)) != 0};
  return !state.controls.disabled && (state.charge >= std::max(definition.energy_thresholds[0], state.setting) || (recent && state.charge >= definition.energy_thresholds[1]));
}

bool ammunition_empty(weapon_controls const &state, int16_t const ammunition) {
  return state.disabled || ammunition == 0;
}

bool ammunition_ready(weapon_controls const &state, int16_t const ammunition) {
  return !state.disabled && state.refire_remaining == 0 && ammunition != 0;
}

void advance_weapon_controls(weapon_controls &state, frame_ratio const ratio) {
  if(state.disabled) return;
  state.hud_changes &= state.refreshed_changes;
  state.refreshed_changes = 0;
  countdown(state.refire_remaining, ratio);
  if(state.firing_animation) {
    advance_cell(state);
    if(state.cell == 0) state.firing_animation = false;
  }
}

int16_t charge_capacitor(capacitor &state, bool const inhibited, int16_t const budget, frame_ratio const ratio) {
  if(state.controls.disabled) return budget;
  advance_weapon_controls(state.controls, ratio);
  auto const difference{wrap_word((inhibited ? 0 : state.setting) - state.charge)};
  int16_t amount;
  if(difference <= 0) {
    state.fast = false;
    amount = std::max<int16_t>(-capacitor_discharge_per_update, difference);
  } else {
    amount = std::min({state.fast ? capacitor_fast_charge_per_update : capacitor_charge_per_update, budget, difference});
  }
  state.charge = wrap_word(state.charge + amount);
  return wrap_word(budget - amount);
}

bool poll_elf_trigger(weapon_controls &controls, elf_trigger &trigger, bool const pressed) {
  if(trigger.ready) {
    if(!pressed) {
      controls.cell = 0;
      trigger.ready = false;
    }
    return pressed;
  }
  if(!pressed && trigger.spinning) {
    controls.cell = 0;
    trigger.spinning = false;
  } else if(pressed && !trigger.spinning) {
    trigger.spinning = true;
    trigger.remaining = 0;
  }
  return false;
}

int16_t advance_elf(capacitor &state, elf_trigger &trigger, bool const inhibited, int16_t const budget, frame_ratio const ratio) {
  if(state.controls.disabled) return budget;
  if(trigger.spinning) {
    countdown(trigger.remaining, ratio);
    if(trigger.remaining == 0) {
      if(state.controls.cell + 1U == state.controls.cell_count) {
        trigger.ready = true;
        trigger.spinning = false;
      } else {
        advance_cell(state.controls);
        trigger.remaining = 0;
      }
    }
  }
  return charge_capacitor(state, inhibited, budget, ratio);
}

void change_capacitor_setting(capacitor &state, bool const increase) {
  state.setting = std::clamp<int16_t>(wrap_word(state.setting + (increase ? capacitor_setting_step : -capacitor_setting_step)), 0, capacitor_maximum);
  state.controls.hud_changes |= std::to_underlying(weapon_hud_change::setting);
  state.controls.refreshed_changes |= std::to_underlying(weapon_hud_change::setting);
}

void default_capacitor_setting(capacitor &state) {
  if(!state.controls.disabled) state.setting = capacitor_default_setting;
}

void fast_charge_capacitor(capacitor &state) {
  state.fast = true;
  state.setting = capacitor_maximum;
}

int16_t allocate_capacitors(std::span<capacitor_slot const> const slots, int16_t const selected, int16_t budget, frame_ratio const ratio, bool const free_energy) {
  if(selected >= 0 && static_cast<size_t>(selected) >= slots.size()) throw std::out_of_range{"selected capacitor slot"};
  auto const initial_budget{budget};
  std::vector<bool> visited(slots.size(), false);
  bool inhibited{false};
  for(size_t count{0}; count != slots.size(); ++count) {
    size_t next{slots.size()};
    if(selected >= 0 && !visited[static_cast<size_t>(selected)]) {
      next = static_cast<size_t>(selected);
    } else {
      for(size_t i{0}; i != slots.size(); ++i) {
        if(!visited[i] && slots[i].energy && (next == slots.size() || priority(*slots[i].energy) > priority(*slots[next].energy))) next = i;
      }
    }
    if(next == slots.size() || !slots[next].energy) break;
    visited[next] = true;
    auto const &slot{slots[next]};
    budget = slot.elf ? advance_elf(*slot.energy, *slot.elf, inhibited, budget, ratio) : charge_capacitor(*slot.energy, inhibited, budget, ratio);
    if(slot.energy->fast) inhibited = true;
  }
  return free_energy ? initial_budget : budget;
}

} // namespace earthsiege
