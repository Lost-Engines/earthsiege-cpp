#pragma once
#include <optional>
#include <span>
#include <vector>
#include "resources/combat.h"
#include "resources/model.h"
#include "resources/weapon_mounts.h"

namespace earthsiege {

class weapon_bank;

struct mounted_weapon_model {
  int16_t body_part{0};
  int16_t weapon{empty_weapon_slot};
  int8_t equipment_slot{0};
  bool disabled{false};
  std::optional<model_part> geometry;
  std::vector<uint16_t> cell_frames;
};

/// Own weapon geometry cloned for one loadout, sharing its owner's articulated skeleton
class weapon_assembly {
  std::vector<mounted_weapon_model> models;

public:
  weapon_assembly(model_part const &owner, std::span<weapon_mount const> mounts, std::span<int16_t const> equipment, weapon_catalogue const &definitions, model_resource const &templates);
  std::span<mounted_weapon_model const> parts() const;
  void disable_missing_sections(std::span<int16_t const> presence);
  void update_cells(weapon_bank const &bank);
};

} // namespace earthsiege
