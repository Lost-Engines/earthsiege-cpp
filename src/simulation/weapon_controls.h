#pragma once
#include <span>
#include "resources/combat.h"
#include "simulation/frame_time.h"

namespace earthsiege {

inline int16_t constexpr capacitor_maximum{1200};
inline int16_t constexpr capacitor_setting_step{80};
inline int16_t constexpr capacitor_default_setting{820};
inline int16_t constexpr capacitor_charge_per_update{20};
inline int16_t constexpr capacitor_fast_charge_per_update{60};
inline int16_t constexpr capacitor_discharge_per_update{5};

enum class weapon_hud_change : uint8_t {
  fired = 1U << 0,
  setting = 1U << 1,
};

struct weapon_controls {
  bool disabled{false};
  int16_t refire_remaining{0};
  // Original eight HUD change latches and their one-update retention mask.
  uint8_t hud_changes{0};
  uint8_t refreshed_changes{0};
  uint16_t cell{0};
  uint16_t cell_count{1};
  bool firing_animation{false};
};

struct capacitor {
  weapon_controls controls;
  int16_t setting{0};
  int16_t charge{0};
  bool fast{false};
};

struct elf_trigger {
  bool spinning{false};
  bool ready{false};
  int16_t remaining{0};
};

struct capacitor_slot {
  capacitor *energy{nullptr};
  elf_trigger *elf{nullptr};
};

bool weapon_in_range(weapon_definition const &definition, int32_t distance);
bool energy_ready(weapon_definition const &definition, capacitor const &state);
bool capacitor_charged(weapon_definition const &definition, capacitor const &state);
bool ammunition_empty(weapon_controls const &state, int16_t ammunition);
bool ammunition_ready(weapon_controls const &state, int16_t ammunition);
void advance_weapon_controls(weapon_controls &state, frame_ratio ratio);
int16_t charge_capacitor(capacitor &state, bool inhibited, int16_t budget, frame_ratio ratio);
bool poll_elf_trigger(weapon_controls &controls, elf_trigger &trigger, bool pressed);
int16_t advance_elf(capacitor &state, elf_trigger &trigger, bool inhibited, int16_t budget, frame_ratio ratio);
void change_capacitor_setting(capacitor &state, bool increase);
void default_capacitor_setting(capacitor &state);
void fast_charge_capacitor(capacitor &state);
/// Selected slot first, then stable descending priority; a selected missing slot ends the pass
int16_t allocate_capacitors(std::span<capacitor_slot const> slots, int16_t selected, int16_t budget, frame_ratio ratio, bool free_energy);

} // namespace earthsiege
