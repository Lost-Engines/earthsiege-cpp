#pragma once
#include <functional>
#include <vector>
#include "resources/combat.h"
#include "resources/player_roster.h"

namespace earthsiege {

struct damage_consequences {
  bool dead{false};
  bool disabled{false};
  bool movement_light{false};
  bool movement_heavy{false};
  bool reactor_light{false};
  bool reactor_heavy{false};
  bool operator==(damage_consequences const &) const = default;
};

struct damage_state {
  std::vector<int16_t> internal;
  std::vector<int16_t> armour;
  std::vector<int16_t> present;
  std::vector<int16_t> cells;
  damage_consequences consequences;
};

/// Mutate signed-word accumulation and remaining damage; negative accumulation means destroyed
bool accumulate_damage(int16_t &amount, int16_t capacity, int16_t &current);

using component_choice = std::function<int16_t(int16_t)>;
using section_detachment = std::function<void(size_t)>;

/// Armor, weighted internal damage and section cascades; effects run at the detachment callback
class component_damage {
  damage_definition const &definition;
  damage_state current;
  bool distribute(size_t section, int16_t amount, component_choice const &choose);
  void remove(size_t section, component_choice const &choose, section_detachment const &detach, std::vector<size_t> &pending, std::vector<size_t> &removed);
  void update_consequences(int16_t legs);

public:
  component_damage(damage_definition const &definition, damage_state initial);
  std::vector<size_t> hit(size_t section, int16_t amount, int16_t legs, component_choice const &choose, section_detachment const &detach = {});
  /// Raw section damage shared by HERCs and flyers, without HERC-specific consequences.
  std::vector<size_t> apply_section(size_t section, int16_t amount, component_choice const &choose, section_detachment const &detach = {});
  damage_state const &state() const;
  /// Shell percentages restore accumulators without recomputing cells or consequence flags.
  void restore_condition(herc_condition const &condition);
  /// Authored wreck/reactor presets, before the random starting-damage pass.
  void apply_starting_impairments(int16_t condition);
  int16_t section_capacity(size_t section) const;
  /// Q8 total armour/internal damage; reference weights do not weight this measurement
  int16_t fraction(size_t section) const;
  int16_t total_fraction() const;
  /// Original weapon knock-off bypasses the ordinary presence and consequence wrapper
  void knock_off_weapon(size_t section, component_choice const &choose, section_detachment const &detach);
};

} // namespace earthsiege
