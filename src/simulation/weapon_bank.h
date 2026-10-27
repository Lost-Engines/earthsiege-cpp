#pragma once
#include <optional>
#include <variant>
#include <vector>
#include "resources/model.h"
#include "resources/weapon_mounts.h"
#include "simulation/weapon_controls.h"

namespace earthsiege {

struct elf_capacitor {
  capacitor power;
  elf_trigger trigger;
};

struct ammunition_weapon {
  weapon_controls controls;
  int16_t rounds{0};
  bool available{true};
  int32_t displayed_rounds{0};                                              // Q8 HUD countdown is distinct from actual ammunition
};

struct grenade_weapon {
  ammunition_weapon supply;
  bool holding{false};
  int32_t throw_range{0};
};

struct ecm_weapon {
  weapon_controls controls;
};

struct weapon_state {
  int16_t definition{0};
  int8_t equipment_slot{0};
  std::optional<size_t> projectile;
  std::variant<capacitor, elf_capacitor, ammunition_weapon, grenade_weapon, ecm_weapon> mechanism;
  weapon_controls &controls();
  weapon_controls const &controls() const;
  capacitor *energy();
  capacitor const *energy() const;
  ammunition_weapon *ammunition();
  ammunition_weapon const *ammunition() const;
  elf_trigger *elf();
  elf_trigger const *elf() const;
  bool ready(weapon_definition const &source) const;
};

/// Original GL mount order, including empty slots and internal equipment
class weapon_bank {
  weapon_catalogue const &source;
  std::vector<std::optional<weapon_state>> weapons;

public:
  weapon_bank(weapon_catalogue const &definitions, model_resource const &templates, std::span<weapon_mount const> mounts, std::span<int16_t const> equipment, std::span<int16_t const> values = {});
  std::span<std::optional<weapon_state> const> slots() const;
  weapon_state &at(size_t slot);
  void disable_missing_sections(std::span<int16_t const> presence);
  int16_t allocate(int16_t selected, int16_t budget, frame_ratio ratio, bool free_energy = false);
};

} // namespace earthsiege
