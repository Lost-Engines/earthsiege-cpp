#pragma once
#include <string>
#include "simulation/weapon_bank.h"

namespace earthsiege {

enum class weapon_instrument_kind : uint8_t { energy, ammunition, ecm };
struct weapon_instrument_state {
  int32_t value{};                                                         // Q10 capacitor charge, or displayed ammunition count
  int32_t requested_charge{};                                              // Q10, owned by the slider unless a keyboard change is pending
  bool disabled{};
  bool selected{};                                                         // ECM uses this bit for its on/off state
  bool included{};
  bool ready{};
};
struct weapon_display {
  size_t bank_slot;
  unsigned int control_number;
  weapon_instrument_kind kind;
  std::string name;
  weapon_instrument_state state;
};
weapon_instrument_kind instrument_kind(weapon_state const &weapon);
std::string weapon_display_name(weapon_state const &weapon, weapon_catalogue const &definitions);
/// Original 0xb5b8/0xb67c/0xb588 exchanges; separate from simulation charging/firing.
void exchange_weapon_display(weapon_state &weapon, weapon_instrument_state &display, bool ready, bool included, bool selected, frame_ratio ratio);

} // namespace earthsiege
