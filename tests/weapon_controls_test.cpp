#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include "resources/resource_store.h"
#include "simulation/herc_power.h"
#include "simulation/shield_power.h"
#include "simulation/weapon_controls.h"

int main(int const argc, char **argv) try {
  if(argc != 3) return EXIT_FAILURE;
  earthsiege::resource_store resources{argv[1]};
  auto const catalogue{earthsiege::read_weapon_catalogue(resources.read("simvol0", "dat/WEAPONS.DAT"))};
  auto const bytes{earthsiege::read_file(argv[2])};
  earthsiege::binary_reader file{bytes};
  auto const magic{file.take(8)};
  if(std::string(magic.begin(), magic.end()) != "ESPOWER1") file.fail("fixture version");
  auto const count{file.integer<uint32_t>()};
  earthsiege::capacitor elf_weapon;
  earthsiege::elf_trigger elf;
  unsigned int elf_step{0};
  for(unsigned int row{0}; row != count; ++row) {
    auto const kind{file.integer<uint8_t>()};
    auto const size{file.integer<uint16_t>()};
    std::vector<int32_t> v(size);
    for(auto &value : v) value = file.integer<int32_t>();
    auto word{[&](size_t const index){ return static_cast<int16_t>(v.at(index)); }};
    auto byte{[&](size_t const index){ return static_cast<uint8_t>(v.at(index)); }};
    auto flag{[&](size_t const index){ return v.at(index) != 0; }};
    auto check{[&](int32_t const actual, size_t const index){
      if(actual != v.at(index)) file.fail("weapon/power row " + std::to_string(row) + " kind " + std::to_string(kind) + " field " + std::to_string(index) + ": " + std::to_string(actual) + " != " + std::to_string(v.at(index)));
    }};
    if(kind == 0) {
      check(earthsiege::weapon_in_range(catalogue.weapons.at(static_cast<size_t>(v[0])), v[1]), 2);
    } else if(kind == 1) {
      earthsiege::capacitor state{.controls{.disabled{flag(4)}, .refire_remaining{word(3)}, .hud_changes{byte(5)}}, .setting{word(2)}, .charge{word(1)}};
      auto const &definition{catalogue.weapons.at(static_cast<size_t>(v[0]))};
      check(earthsiege::energy_ready(definition, state), 6);
      check(earthsiege::ammunition_empty(state.controls, word(2)), 7);
      check(earthsiege::ammunition_ready(state.controls, word(2)), 8);
      check(earthsiege::capacitor_charged(definition, state), 9);
    } else if(kind == 2) {
      earthsiege::capacitor state{.controls{.disabled{flag(5)}, .refire_remaining{1200}, .hud_changes{0x55}, .refreshed_changes{0x33}}, .setting{word(0)}, .charge{word(1)}, .fast{flag(3)}};
      auto const budget{earthsiege::charge_capacitor(state, flag(4), word(2), word(6))};
      check(state.charge, 7); check(budget, 8); check(state.fast, 9); check(state.controls.refire_remaining, 10);
      if(state.controls.hud_changes != (flag(5) ? 0x55 : 0x11) || state.controls.refreshed_changes != (flag(5) ? 0x33 : 0)) file.fail("charge HUD flags differ");
    } else if(kind == 3) {
      if(elf_step++ % 36 == 0) {
        elf_weapon = {.controls{.cell_count{static_cast<uint16_t>(v[0])}}, .setting{400}, .charge{400}};
        elf = {};
      }
      check(earthsiege::poll_elf_trigger(elf_weapon.controls, elf, flag(2)), 3);
      earthsiege::advance_elf(elf_weapon, elf, false, 0, word(1));
      check(elf.spinning, 4); check(elf.ready, 5); check(elf_weapon.controls.cell, 6);
    } else if(kind == 4) {
      earthsiege::capacitor state{.controls{}, .setting{word(0)}};
      earthsiege::change_capacitor_setting(state, flag(1));
      check(state.setting, 2);
      if(state.controls.hud_changes != 2 || state.controls.refreshed_changes != 2) file.fail("setting HUD flags differ");
    } else if(kind == 5) {
      earthsiege::weapon_controls state{.disabled{flag(0)}, .refire_remaining{word(1)}, .hud_changes{byte(3)}, .refreshed_changes{byte(4)}, .cell{static_cast<uint16_t>(v[5])}, .cell_count{static_cast<uint16_t>(v[6])}, .firing_animation{flag(7)}};
      earthsiege::advance_weapon_controls(state, word(2));
      check(state.refire_remaining, 8); check(state.hud_changes, 9); check(state.refreshed_changes, 10); check(state.cell, 11); check(state.firing_animation, 12);
    } else if(kind == 6) {
      earthsiege::capacitor state{.controls{.disabled{flag(1)}}, .setting{word(0)}};
      if(flag(2)) earthsiege::fast_charge_capacitor(state);
      else earthsiege::default_capacitor_setting(state);
      check(state.setting, 3); check(state.fast, 4);
    } else if(kind == 7) {
      earthsiege::shield_power state{word(0), word(1), word(2), word(3)};
      auto const budget{earthsiege::recharge_shields(state, word(4))};
      check(state.front, 5); check(state.rear, 6); check(budget, 7);
    } else if(kind == 8) {
      earthsiege::shield_power state{.balance{word(0)}};
      earthsiege::change_shield_balance(state, flag(1));
      check(state.balance, 2);
    } else if(kind == 9) {
      std::vector<earthsiege::capacitor> states(static_cast<size_t>(v[3]));
      std::vector<earthsiege::capacitor_slot> slots(states.size());
      for(size_t i{0}; i != states.size(); ++i) {
        size_t const first{4 + i * 7};
        states[i] = {.controls{.disabled{flag(first + 4)}}, .setting{word(first + 1)}, .charge{word(first + 2)}, .fast{flag(first + 3)}};
        if(flag(first)) slots[i].energy = &states[i];
      }
      check(earthsiege::allocate_capacitors(slots, word(0), word(1), 256, flag(2)), v.size() - 1);
      for(size_t i{0}; i != states.size(); ++i) {
        check(states[i].charge, 4 + i * 7 + 5);
        check(states[i].fast, 4 + i * 7 + 6);
      }
    } else if(kind == 10) {
      earthsiege::herc_power state{word(0), {word(4), word(5), word(6), word(7)}};
      earthsiege::damage_consequences damage;
      damage.reactor_light = flag(2); damage.reactor_heavy = flag(3);
      unsigned int calls{0};
      earthsiege::update_herc_power(state, damage, word(1), [&](int16_t const available){ ++calls; check(available, 9); return word(8); });
      check(state.reserve, 10); check(state.shields.front, 11); check(state.shields.rear, 12); check(state.shields.balance, 13); check(state.shields.capacity, 14);
      if(calls != 1) file.fail("reactor weapon allocation count differs");
    } else file.fail("unknown power case");
  }
  file.require_end();
  if(count != 16631 || elf_step != 432) file.fail("weapon/power reference coverage differs");
  std::cout << count << " original weapon control and power allocation cases agree" << std::endl;
  return EXIT_SUCCESS;
} catch(std::exception const &error) {
  std::cerr << error.what() << std::endl;
  return EXIT_FAILURE;
}
