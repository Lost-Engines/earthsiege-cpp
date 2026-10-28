#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include "simulation/weapon_display.h"

int main(int argc, char **argv) try {
  using namespace earthsiege;
  if(argc != 2) return EXIT_FAILURE;
  auto const source{read_file(argv[1])}; binary_reader file{source}; auto const magic{file.take(8)};
  if(std::string(magic.begin(), magic.end()) != "ESWPND01") file.fail("fixture version");
  auto const count{file.integer<uint32_t>()};
  for(unsigned int i{}; i < count; ++i) {
    auto const kind{static_cast<weapon_instrument_kind>(file.integer<uint8_t>())};
    bool const disabled{file.integer<uint8_t>() != 0}, keyboard{file.integer<uint8_t>() != 0};
    bool const selected{file.integer<uint8_t>() != 0}, included{file.integer<uint8_t>() != 0}, ready{file.integer<uint8_t>() != 0};
    auto const setting{file.integer<int32_t>()}, charge{file.integer<int32_t>()}, requested{file.integer<int32_t>()};
    auto const rounds{file.integer<int32_t>()}, displayed{file.integer<int32_t>()}, ratio{file.integer<int32_t>()}, enabled{file.integer<int32_t>()};
    weapon_state weapon;
    weapon_controls controls{.disabled{disabled}, .hud_changes{static_cast<uint8_t>(keyboard ? 2 : 0)}};
    if(kind == weapon_instrument_kind::energy) weapon.mechanism = capacitor{controls,static_cast<int16_t>(setting),static_cast<int16_t>(charge),false};
    else if(kind == weapon_instrument_kind::ammunition) weapon.mechanism = ammunition_weapon{controls,static_cast<int16_t>(rounds),true,displayed};
    else weapon.mechanism = ecm_weapon{controls,false,false};
    weapon_instrument_state state{.requested_charge{requested}, .disabled{kind == weapon_instrument_kind::ecm && disabled}, .selected{kind == weapon_instrument_kind::ecm && enabled != 0}};
    exchange_weapon_display(weapon,state,ready,included,selected,static_cast<frame_ratio>(ratio));
    for(int const value : {state.value,state.requested_charge,int(state.disabled),int(state.selected),int(state.included),int(state.ready)})
      if(value != file.integer<int32_t>()) file.fail("weapon display state differs at case " + std::to_string(i));
    auto const expected_setting{file.integer<int32_t>()}, expected_displayed{file.integer<int32_t>()}, expected_pulse{file.integer<int32_t>()};
    if(auto const *power{weapon.energy()}) { if(power->setting != expected_setting) file.fail("energy setting differs"); }
    else if(auto const *ammo{weapon.ammunition()}) {
      if(ammo->rounds != expected_setting || ammo->displayed_rounds != expected_displayed) file.fail("ammo countdown differs");
    } else {
      auto const &ecm{std::get<ecm_weapon>(weapon.mechanism)};
      if(ecm.enabled != bool(expected_setting) || ecm.pulse != bool(expected_pulse)) file.fail("ECM state differs");
    }
  }
  file.require_end();
  std::cout << count << " original weapon/display exchanges match\n";
  return EXIT_SUCCESS;
} catch(std::exception const &error) { std::cerr << error.what() << '\n'; return EXIT_FAILURE; }
