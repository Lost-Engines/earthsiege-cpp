#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include "resources/herc_definition.h"
#include "resources/resource_store.h"
#include "simulation/herc_actor.h"

int main(int const argc, char **argv) try {
  if(argc != 3) return EXIT_FAILURE;
  earthsiege::resource_store resources{argv[1]};
  auto const definitions{earthsiege::read_weapon_catalogue(resources.read("simvol0", "dat/WEAPONS.DAT"))};
  auto const templates{earthsiege::read_model(resources.read("simvol0", "dts/MECHWPNS.DTS"))};
  auto const bytes{earthsiege::read_file(argv[2])};
  earthsiege::binary_reader file{bytes};
  auto const magic{file.take(8)};
  if(std::string(magic.begin(), magic.end()) != "ESWBANK2") file.fail("fixture version");
  auto const count{file.integer<uint32_t>()};
  unsigned int updates{0}, reactor_updates{0};
  auto const shorts{[&]{
    std::vector<int16_t> values(file.integer<uint16_t>());
    for(auto &value : values) value = file.integer<int16_t>();
    return values;
  }};
  for(unsigned int row{0}; row != count; ++row) {
    auto const name{file.cstring()};
    auto const equipment{shorts()};
    auto const values{shorts()};
    auto const body{earthsiege::read_herc_definition(resources, name)};
    earthsiege::herc_actor actor{body, definitions, templates, equipment, {}, {}, values};
    auto &bank{actor.weapons()};
    auto const check{[&](int const step){
      auto const slots{bank.slots()};
      if(file.integer<uint16_t>() != slots.size()) file.fail("weapon count differs");
      for(size_t i{0}; i != slots.size(); ++i) {
        auto const &state{slots[i]};
        if((file.integer<uint8_t>() != 0) != state.has_value()) file.fail("empty weapon slot differs");
        if(!state) continue;
        auto const &control{state->controls()};
        auto const *energy{state->energy()};
        auto const *ammo{state->ammunition()};
        auto const *elf{state->elf()};
        std::array<int32_t, 18> const actual{
          state->definition, state->equipment_slot, state->projectile ? static_cast<int32_t>(*state->projectile) : -1,
          control.disabled, control.refire_remaining, control.hud_changes, control.refreshed_changes,
          control.cell, control.cell_count, energy ? energy->fast : 0, control.firing_animation,
          state->ready(definitions.weapons.at(static_cast<size_t>(state->definition))),
          energy ? (energy->fast ? 10000 : energy->setting) : 0,
          energy ? energy->setting : -1, energy ? energy->charge : -1, ammo ? ammo->rounds : -1,
          elf ? elf->spinning : -1, elf ? elf->ready : -1,
        };
        for(size_t field{0}; field != actual.size(); ++field) {
          auto const expected{file.integer<int32_t>()};
          if(actual[field] != expected) file.fail(name + '/' + std::to_string(row) + '/' + std::to_string(step) + '/' + std::to_string(i) + " field " + std::to_string(field) + ": " + std::to_string(actual[field]) + " != " + std::to_string(expected));
        }
      }
    }};
    check(-1);
    auto const steps{file.integer<uint16_t>()};
    for(unsigned int step{0}; step != steps; ++step) {
      auto const ratio{file.integer<int16_t>()};
      auto const budget{file.integer<int16_t>()};
      auto const selected{file.integer<int16_t>()};
      bool const free{file.integer<int16_t>() != 0};
      auto const changed{file.integer<int16_t>()};
      auto const action{file.integer<int16_t>()};
      auto const disabled{file.integer<int16_t>()};
      auto const expected{file.integer<int16_t>()};
      if(changed >= 0) {
        auto *energy{bank.at(static_cast<size_t>(changed)).energy()};
        if(!energy) file.fail("charge setting for non-capacitor");
        if(action == 1 || action == 2) earthsiege::change_capacitor_setting(*energy, action == 2);
        else if(action == 3) earthsiege::default_capacitor_setting(*energy);
        else if(action == 4) earthsiege::fast_charge_capacitor(*energy);
        else file.fail("unknown charge action");
      }
      if(disabled >= 0) bank.at(static_cast<size_t>(disabled)).controls().disabled = true;
      auto const result{bank.allocate(selected, budget, ratio, free)};
      if(result != expected) file.fail("mixed bank returned budget differs");
      check(static_cast<int>(step));
      ++updates;
    }
    auto const check_power{[&]{
      auto const &power{actor.power()};
      std::array<int16_t, 5> const actual{power.reserve, power.shields.front, power.shields.rear, power.shields.balance, power.shields.capacity};
      for(auto const value : actual) if(value != file.integer<int16_t>()) file.fail(name + ": actor power differs");
    }};
    check_power();
    auto const reactor_steps{file.integer<uint16_t>()};
    for(unsigned int step{0}; step != reactor_steps; ++step) {
      auto const ratio{file.integer<int16_t>()};
      bool const free{file.integer<int16_t>() != 0};
      actor.update_power(ratio, free);
      check_power();
      check(static_cast<int>(steps + step));
      for(auto const &model : actor.body().weapons().parts()) for(auto const &weapon : bank.slots()) {
        if(!weapon || weapon->equipment_slot != model.equipment_slot) continue;
        if(model.disabled != weapon->controls().disabled || (!model.cell_frames.empty() && model.cell_frames.front() != weapon->controls().cell)) file.fail("live weapon model state differs");
      }
      ++reactor_updates;
    }
  }
  file.require_end();
  if(count != 34 || updates != 2720 || reactor_updates != 1360) file.fail("weapon bank reference coverage differs");
  std::cout << count << " original weapon constructors and " << updates << " mixed allocation passes and " << reactor_updates << " owned reactor updates agree" << std::endl;
  return EXIT_SUCCESS;
} catch(std::exception const &error) {
  std::cerr << error.what() << std::endl;
  return EXIT_FAILURE;
}
