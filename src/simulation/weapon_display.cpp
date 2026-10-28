#include "weapon_display.h"
#include <algorithm>
#include <array>
#include <utility>
#include "simulation/fixed_math.h"

namespace earthsiege {

weapon_instrument_kind instrument_kind(weapon_state const &weapon) {
  if(weapon.energy()) return weapon_instrument_kind::energy;
  if(weapon.ammunition()) return weapon_instrument_kind::ammunition;
  return weapon_instrument_kind::ecm;
}
std::string weapon_display_name(weapon_state const &weapon, weapon_catalogue const &definitions) {
  // Original executable tables 0x911fc (weapon names) and 0x910ac (seekers).
  std::array<std::string_view, original_weapon_count> constexpr names{
    "ATC20", "ATC35", "ATC50", "ELF", "EMP", "GREN", "LAS100", "LAS200", "LAS300",
    "6", "8", "10", "PBEAM", "ECM", "EMP", "PBEAM", "MISSL",
  };
  std::array<std::string_view, 4> constexpr seekers{"SARH", "ARH", "ARM", "EO"};
  std::string result;
  if(weapon.projectile) {
    auto const &projectile{definitions.projectiles.at(*weapon.projectile)};
    if(projectile.kind == projectile_kind::rocket) result = seekers.at(static_cast<size_t>(projectile.subtype));
  }
  result += names.at(static_cast<size_t>(weapon.definition));
  return result;
}
void exchange_weapon_display(weapon_state &weapon, weapon_instrument_state &display, bool const ready, bool const included, bool const selected, frame_ratio const ratio) {
  if(auto *ecm{std::get_if<ecm_weapon>(&weapon.mechanism)}) {
    // The original ECM exchange copies the UI switch to both device flags.
    ecm->enabled = ecm->pulse = display.selected;
    return;
  }
  if(auto *energy{weapon.energy()}) {
    if(energy->controls.hud_changes & std::to_underlying(weapon_hud_change::setting))
      display.requested_charge = energy->setting * 1024 / capacitor_maximum;
    else energy->setting = wrap_word(wrap_long(static_cast<int64_t>(display.requested_charge) * capacitor_maximum) >> 10);
    display.value = energy->charge * 1024 / capacitor_maximum;
  } else if(auto *ammunition{weapon.ammunition()}) {
    int32_t const actual{ammunition->rounds * 256};
    if(ammunition->displayed_rounds > actual) ammunition->displayed_rounds -= scale_frame_time(250, ratio);
    ammunition->displayed_rounds = std::max(actual, ammunition->displayed_rounds);
    display.value = ammunition->displayed_rounds >> 8;
  }
  display.disabled = weapon.controls().disabled;
  display.selected = selected;
  display.included = included;
  display.ready = ready;
}

} // namespace earthsiege
