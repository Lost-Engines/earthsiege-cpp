#include "weapon_bank.h"
#include <stdexcept>
#include <type_traits>
#include <utility>
#include "simulation/herc_loadout.h"

namespace earthsiege {
namespace {

// The constructor dispatches on weapon ID, independently of its projectile selector.
std::variant<capacitor, elf_capacitor, ammunition_weapon, grenade_weapon, ecm_weapon> make_mechanism(int16_t const weapon, int16_t const magazine) {
  capacitor energy{.controls{.hud_changes{2}, .refreshed_changes{2}}, .setting{capacitor_initial_charge}, .charge{capacitor_initial_charge}};
  ammunition_weapon rounds{.controls{}, .rounds{magazine}, .displayed_rounds{static_cast<int32_t>(magazine) * 256}};
  switch(weapon) {
    case 0: case 1: case 2: case 9: case 10: case 11: case 16: return rounds;
    case 3: return elf_capacitor{energy, {}};
    case 4: case 6: case 7: case 8: case 12: case 14: case 15: return energy;
    case 5: return grenade_weapon{rounds, false, 0};
    case 13: return ecm_weapon{};
    default: throw std::out_of_range{"weapon mechanism"};
  }
}

template<typename State> decltype(auto) get_controls(State &state) {
  return std::visit([](auto &value) -> decltype(auto) {
    using type = std::remove_cvref_t<decltype(value)>;
    if constexpr(std::is_same_v<type, elf_capacitor>) return (value.power.controls);
    else if constexpr(std::is_same_v<type, grenade_weapon>) return (value.supply.controls);
    else return (value.controls);
  }, state.mechanism);
}

template<typename State> auto get_energy(State &state) {
  if(auto *value{std::get_if<elf_capacitor>(&state.mechanism)}) return &value->power;
  return std::get_if<capacitor>(&state.mechanism);
}

template<typename State> auto get_ammunition(State &state) {
  if(auto *value{std::get_if<grenade_weapon>(&state.mechanism)}) return &value->supply;
  return std::get_if<ammunition_weapon>(&state.mechanism);
}

} // anonymous namespace

weapon_controls &weapon_state::controls() { return get_controls(*this); }
weapon_controls const &weapon_state::controls() const { return get_controls(*this); }
capacitor *weapon_state::energy() { return get_energy(*this); }
capacitor const *weapon_state::energy() const { return get_energy(*this); }
ammunition_weapon *weapon_state::ammunition() { return get_ammunition(*this); }
ammunition_weapon const *weapon_state::ammunition() const { return get_ammunition(*this); }
elf_trigger *weapon_state::elf() { auto *value{std::get_if<elf_capacitor>(&mechanism)}; return value ? &value->trigger : nullptr; }
elf_trigger const *weapon_state::elf() const { auto const *value{std::get_if<elf_capacitor>(&mechanism)}; return value ? &value->trigger : nullptr; }

bool weapon_state::ready(weapon_definition const &definition_data) const {
  if(auto const *value{energy()}) return elf() ? capacitor_charged(definition_data, *value) : energy_ready(definition_data, *value);
  if(auto const *value{ammunition()}) return ammunition_ready(value->controls, value->rounds);
  return false;
}

weapon_bank::weapon_bank(weapon_catalogue const &definitions, model_resource const &templates, std::span<weapon_mount const> const mounts, std::span<int16_t const> const equipment, std::span<int16_t const> const values) : source{definitions} {
  weapons.reserve(mounts.size());
  for(auto const &mount : mounts) {
    if(mount.equipment_slot < 0 || static_cast<size_t>(mount.equipment_slot) >= equipment.size()) throw std::out_of_range{"weapon equipment slot"};
    auto const slot{static_cast<size_t>(mount.equipment_slot)};
    auto const selected{equipment[slot]};
    if(selected < 0 || selected == empty_weapon_slot) { weapons.emplace_back(std::nullopt); continue; }
    auto const &definition{source.weapons.at(static_cast<size_t>(selected))};
    weapon_state state{.definition{selected}, .equipment_slot{mount.equipment_slot}, .projectile{std::nullopt}, .mechanism{make_mechanism(selected, definition.magazine)}, .convergence{}};
    if(definition.projectile == 21) {
      if(!values.empty() && slot >= values.size()) throw std::out_of_range{"weapon ammunition selection"};
      auto const subtype{values.empty() || values[slot] == 5 ? 0 : values[slot]};
      for(size_t i{0}; i != source.projectiles.size(); ++i) if(source.projectiles[i].kind == projectile_kind::rocket && source.projectiles[i].subtype == subtype) { state.projectile = i; break; }
      if(!state.projectile) throw std::out_of_range{"missile ammunition subtype"};
    } else if(definition.projectile != 20 && definition.projectile != 22) {
      if(definition.projectile < 0 || static_cast<size_t>(definition.projectile) >= source.projectiles.size()) throw std::out_of_range{"weapon projectile"};
      state.projectile = static_cast<size_t>(definition.projectile);
    }
    if(mount.external()) {
      auto const &shape{std::get<shape_part>(templates.roots.at(static_cast<size_t>(definition.model_indices.at(mount.orientation))).content)};
      state.controls().cell_count = shape.cell_frame_counts.empty() ? 0 : shape.cell_frame_counts.front();
    }
    weapons.emplace_back(std::move(state));
  }
}

std::span<std::optional<weapon_state> const> weapon_bank::slots() const { return weapons; }
weapon_state &weapon_bank::at(size_t const slot) { return weapons.at(slot).value(); }

void weapon_bank::disable_missing_sections(std::span<int16_t const> const presence) {
  for(auto &weapon : weapons) if(weapon) {
    auto const section{weapon_section_first + static_cast<size_t>(weapon->equipment_slot)};
    if(section >= presence.size()) throw std::out_of_range{"weapon damage section"};
    if(presence[section] == 0) weapon->controls().disabled = true;
  }
}

int16_t weapon_bank::allocate(int16_t const selected, int16_t const budget, frame_ratio const ratio, bool const free_energy) {
  std::vector<weapon_power_slot> slots;
  slots.reserve(weapons.size());
  for(auto &weapon : weapons) {
    if(!weapon) slots.push_back({});
    else slots.push_back({.energy{weapon->energy()}, .elf{weapon->elf()}, .ordinary{weapon->energy() ? nullptr : &weapon->controls()}});
  }
  return allocate_weapon_power(slots, selected, budget, ratio, free_energy);
}

} // namespace earthsiege
