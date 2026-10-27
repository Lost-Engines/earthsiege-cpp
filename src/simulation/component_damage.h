#pragma once
#include <functional>
#include <vector>
#include "resources/combat.h"

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
  damage_state const &state() const;
};

} // namespace earthsiege
