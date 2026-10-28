#include "player_armament.h"
#include <stdexcept>
#include "simulation/fixed_math.h"
#include "simulation/herc_weapon_selection.h"

namespace earthsiege {

player_armament::player_armament(weapon_bank &weapons, weapon_catalogue const &catalogue, std::span<weapon_mount const> const attachment_points)
  : bank{weapons}, definitions{catalogue}, mounts{attachment_points}, pairs(weapons.slots().size(), false) {
  if(mounts.size() != bank.slots().size()) throw std::invalid_argument{"player weapon mount count"};
  for(auto &entries : groups) entries.resize(mounts.size(), false);
  for(size_t slot{0}; slot != mounts.size(); ++slot) if(auto const &weapon{bank.slots()[slot]}; weapon && !std::holds_alternative<ecm_weapon>(weapon->mechanism)) {
    groups[0][slot] = true;
    if(selected < 0) selected = static_cast<int16_t>(slot);
  }
}
int16_t player_armament::selection() const { return selected; }
uint8_t player_armament::current_group() const { return group; }
bool player_armament::included(size_t const index, size_t const slot) const { return groups.at(index).at(slot); }
bool player_armament::linked(size_t const slot) const { return pairs.at(slot); }
bool player_armament::holds_selection() const { return held_selection; }
bool player_armament::auto_tracking() const { return tracking; }
bool player_armament::manual_launched() const { return manual_fired; }
bool player_armament::manual_guidance() const { return manual_active; }
void player_armament::set_tracking(bool const enabled) { tracking = enabled; }
void player_armament::hold_selection(bool const held) { held_selection = held; }
void player_armament::set_manual_guidance(bool const active) { manual_active = active; }

bool player_armament::available(size_t const slot) const {
  auto const &weapon{bank.slots()[slot]};
  if(!weapon || std::holds_alternative<ecm_weapon>(weapon->mechanism)) return false;
  auto const *ammunition{weapon->ammunition()};
  return !ammunition || ammunition->available;
}
bool player_armament::ready(size_t const slot) const {
  auto const &weapon{bank.slots()[slot]};
  return weapon && weapon->ready(definitions.weapons.at(static_cast<size_t>(weapon->definition)));
}
bool player_armament::select(size_t const slot) {
  if(slot >= mounts.size() || !bank.slots()[slot]) return false;
  if(selected != static_cast<int16_t>(slot)) {
    selected = static_cast<int16_t>(slot);
    if(pairs[slot] && mounts[slot].paired_slot_offset < 0) selected += mounts[slot].paired_slot_offset;
    manual_fired = false;
  }
  return true;
}
void player_armament::cycle(int16_t const direction) {
  if(selected < 0) return;
  auto next{selected};
  do {
    next += direction;
    if(next < 0) next = static_cast<int16_t>(mounts.size() - 1);
    else if(static_cast<size_t>(next) >= mounts.size()) next = 0;
    auto const slot{static_cast<size_t>(next)};
    if(available(slot) && (!pairs[slot] || mounts[slot].paired_slot_offset > 0) && groups[group][slot]) break;
  } while(next != selected);
  select(static_cast<size_t>(next));
}
void player_armament::change_group(uint8_t const index) {
  if(index >= groups.size()) throw std::out_of_range{"player weapon group"};
  if(index == group) return;
  group = index;
  if(selected >= 0 && !groups[group][static_cast<size_t>(selected)]) cycle();
}
void player_armament::cycle_group() {
  // The keyboard handler writes the group before calling its change helper.
  // Consequently B leaves selection in place until the next readiness update.
  group = static_cast<uint8_t>((group + 1) % groups.size());
}
void player_armament::toggle_inclusion(size_t const slot) { groups.at(group).at(slot).flip(); }
bool player_armament::toggle_category(int8_t const category) {
  for(size_t slot{0}; slot != mounts.size(); ++slot) if(auto const &weapon{bank.slots()[slot]}; weapon && mounts[slot].control_number == category && definitions.weapons.at(static_cast<size_t>(weapon->definition)).range > 0) {
    toggle_inclusion(slot);
    if(auto *energy{bank.at(slot).energy()}; energy && !energy->fast && energy->setting == 0) default_capacitor_setting(*energy);
    return true;
  }
  return false;
}
void player_armament::toggle_pair() {
  if(selected < 0) return;
  auto const slot{static_cast<size_t>(selected)};
  auto const offset{mounts[slot].paired_slot_offset};
  if(offset == 0) return;
  auto const other{static_cast<int>(selected) + offset};
  if(other < 0 || static_cast<size_t>(other) >= mounts.size()) throw std::out_of_range{"paired weapon mount"};
  auto const partner{static_cast<size_t>(other)};
  if(!bank.slots()[partner] || bank.at(slot).definition != bank.at(partner).definition) return;
  pairs[slot].flip();
  pairs[partner].flip();
}
void player_armament::refresh_pairs() {
  for(size_t slot{0}; slot != mounts.size(); ++slot) if(pairs[slot] && (bank.at(slot).controls().disabled || !available(slot))) {
    auto const partner{static_cast<size_t>(static_cast<int>(slot) + mounts[slot].paired_slot_offset)};
    pairs.at(slot).flip();
    pairs.at(partner).flip();
    select(partner);
  }
}
void player_armament::default_charge() {
  for(size_t slot{0}; slot != mounts.size(); ++slot) if(bank.slots()[slot]) if(auto *energy{bank.at(slot).energy()}) default_capacitor_setting(*energy);
}
void player_armament::fast_charge() {
  for(size_t slot{0}; slot != mounts.size(); ++slot) if(bank.slots()[slot]) if(auto *energy{bank.at(slot).energy()}) {
    if(static_cast<int16_t>(slot) == selected) fast_charge_capacitor(*energy);
    else energy->fast = false;
  }
}
void player_armament::adjust_charge(bool const increase) {
  if(selected >= 0) if(auto *energy{bank.at(static_cast<size_t>(selected)).energy()}) change_capacitor_setting(*energy, increase);
}
bool player_armament::selectable(size_t const slot, int32_t const distance, std::array<bool, 5> const &locks) const {
  if(!ready(slot)) return false;
  auto const &weapon{bank.at(slot)};
  if(distance != 0 && !weapon_in_range(definitions.weapons.at(static_cast<size_t>(weapon.definition)), distance)) return false;
  auto const channel{weapon_lock_channel(weapon, definitions)};
  return !channel || *channel == 3 || locks.at(*channel);
}
void player_armament::update_selection(int32_t const distance, std::array<bool, 5> const &locks) {
  if(selected < 0 || held_selection || manual_active || selectable(static_cast<size_t>(selected), distance, locks)) return;
  auto const previous{selected};
  size_t attempts{0};
  do {
    cycle();
    if(selectable(static_cast<size_t>(selected), distance, locks)) { held_selection = false; return; }
    if(selected == previous) break;
  } while(attempts++ <= mounts.size());
  selected = previous;
}
bool player_armament::poll_trigger(size_t const slot, bool const pressed, bool &release) const {
  auto &weapon{bank.at(slot)};
  if(auto *elf{weapon.elf()}) return poll_elf_trigger(weapon.controls(), *elf, pressed);
  if(auto *grenade{std::get_if<grenade_weapon>(&weapon.mechanism)}) {
    if(!grenade->holding) {
      if(pressed) { grenade->holding = true; grenade->throw_range = 20000; }
      return false;
    }
    if(pressed) {
      grenade->throw_range = wrap_long(static_cast<int64_t>(grenade->throw_range) + 1000);
      if(grenade->throw_range <= 100000) return false;
      release = true;
    }
    return true;
  }
  return pressed;
}
bool player_armament::fire(bool const pressed, std::function<void(size_t)> const &discharge) {
  if(selected < 0) return false;
  auto const slot{static_cast<size_t>(selected)};
  std::optional<size_t> partner;
  if(pairs[slot]) partner = static_cast<size_t>(static_cast<int>(slot) + mounts[slot].paired_slot_offset);
  if(!ready(slot) || (partner && !ready(*partner))) return false;
  bool release{false};
  if(!poll_trigger(slot, pressed, release) || (partner && !poll_trigger(*partner, pressed, release))) return release;
  discharge(slot);
  if(!ready(slot)) held_selection = false;
  if(weapon_lock_channel(bank.at(slot), definitions) == 3) { manual_fired = true; manual_active = true; }
  if(partner) {
    discharge(*partner);
    // The original rechecks the first weapon here, including for linked pairs.
    if(!ready(slot)) held_selection = false;
  }
  return release;
}
bool player_armament::command(uint16_t const code) {
  switch(code) {
    case 44: default_charge(); break;                                        // Z
    case 48: cycle_group(); break;                                           // B
    case 17: cycle(); held_selection = false; break;                          // W
    case 529: cycle(-1); held_selection = false; break;                       // Alt+W
    case 12: case 13: case 74: case 78: adjust_charge(code == 13 || code == 78); break;
    default:
      if(code < 514 || code > 522) return false;
      toggle_category(static_cast<int8_t>(code - 514));                       // Alt+1..9
  }
  return true;
}

} // namespace earthsiege
