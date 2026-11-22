#pragma once
#include "shell/armament.h"
#include "shell/hangar_service.h"
namespace earthsiege {
class construction_menu {
  int16_t chassis_{0};
  bool enabled_{false};
  bool cycling_{false};
  service_prices const *prices;
public:
  construction_menu(hangar &, career_state const &, service_prices const &, int16_t last = 0);
  void cycle(career_state const &, int direction);
  void activate(hangar &, career_state &, armory &, armament_catalog const &);
  bool building(hangar const &h) const {
    return h.find(h.construction_slot) != nullptr;
  }
  int16_t chassis() const {
    return chassis_;
  }
  bool enabled() const {
    return enabled_;
  }
  bool cycling() const {
    return cycling_;
  }
};
} // namespace earthsiege
