#include <cstdlib>
#include <iostream>
#include <string>
#include "resources/herc_definition.h"
#include "resources/resource_store.h"
#include "simulation/herc_actor.h"
#include "simulation/player_armament.h"

int main(int const argc, char **argv) try {
  if(argc != 3) return EXIT_FAILURE;
  earthsiege::resource_store resources{argv[1]};
  auto const catalogue{earthsiege::read_weapon_catalogue(resources.read("simvol0", "dat/WEAPONS.DAT"))};
  auto const templates{earthsiege::read_model(resources.read("simvol0", "dts/MECHWPNS.DTS"))};
  auto const data{earthsiege::read_file(argv[2])};
  earthsiege::binary_reader file{data};
  auto const magic{file.take(8)};
  if(std::string(magic.begin(), magic.end()) != "ESPARM01") file.fail("fixture version");
  auto const integers{[&]{
    std::vector<int32_t> result(file.integer<uint16_t>());
    for(auto &value : result) value = file.integer<int32_t>();
    return result;
  }};
  auto const shorts{[&]{ auto const values{integers()}; return std::vector<int16_t>(values.begin(), values.end()); }};
  auto const count{file.integer<uint32_t>()};
  unsigned int updates{0}, discharges{0};
  for(unsigned int row{0}; row != count; ++row) {
    auto const name{file.cstring()};
    auto const equipment{shorts()}, values{shorts()};
    auto const source{earthsiege::read_herc_definition(resources, name)};
    earthsiege::herc_actor actor{source, catalogue, templates, equipment, {}, {}, values};
    auto &bank{actor.weapons()};
    earthsiege::player_armament controls{bank, catalogue, source.mounts};
    auto const check{[&](int const step){
      auto const where{name + '/' + std::to_string(row) + '/' + std::to_string(step)};
      std::array<int32_t, 6> const main{controls.selection(), controls.current_group(), controls.holds_selection(), controls.auto_tracking(), controls.manual_launched(), controls.manual_guidance()};
      for(size_t i{0}; i != main.size(); ++i) {
        auto const expected{file.integer<int32_t>()};
        if(main[i] != expected) file.fail(where + " list field " + std::to_string(i) + ": " + std::to_string(main[i]) + " != " + std::to_string(expected));
      }
      for(size_t group{0}; group != 3; ++group) {
        auto const expected{integers()};
        for(size_t slot{0}; slot != expected.size(); ++slot) if(controls.included(group, slot) != (expected[slot] != 0)) file.fail(where + " weapon group differs");
      }
      auto const slots{bank.slots()};
      if(file.integer<uint16_t>() != slots.size()) file.fail(where + " bank size differs");
      for(size_t slot{0}; slot != slots.size(); ++slot) {
        if((file.integer<uint8_t>() != 0) != slots[slot].has_value()) file.fail(where + " empty slot differs");
        if(!slots[slot]) continue;
        auto const &weapon{*slots[slot]}; auto const &control{weapon.controls()};
        auto const *energy{weapon.energy()}; auto const *ammo{weapon.ammunition()}; auto const *elf{weapon.elf()};
        auto const *grenade{std::get_if<earthsiege::grenade_weapon>(&weapon.mechanism)};
        std::array<int32_t, 18> const actual{
          weapon.definition, control.disabled, controls.linked(slot), ammo ? ammo->available : !std::holds_alternative<earthsiege::ecm_weapon>(weapon.mechanism),
          control.refire_remaining, control.hud_changes, control.refreshed_changes, control.cell, control.firing_animation,
          energy ? energy->fast : 0, energy ? energy->setting : -1, energy ? energy->charge : -1, ammo ? ammo->rounds : -1,
          elf ? elf->spinning : -1, elf ? elf->ready : -1, elf ? elf->remaining : -1, grenade ? grenade->holding : -1, grenade ? grenade->throw_range : -1,
        };
        for(size_t i{0}; i != actual.size(); ++i) {
          auto const expected{file.integer<int32_t>()};
          if(actual[i] != expected) file.fail(where + " slot " + std::to_string(slot) + " field " + std::to_string(i) + ": " + std::to_string(actual[i]) + " != " + std::to_string(expected));
        }
      }
    }};
    check(-1);
    auto const steps{file.integer<uint16_t>()};
    for(unsigned int step{0}; step != steps; ++step) {
      auto const action{file.integer<uint16_t>()}; auto const args{integers()};
      auto const expected_result{file.integer<int32_t>()}; bool const expected_release{file.integer<uint8_t>() != 0}; auto const expected_fired{integers()};
      int32_t result{0}; bool release{false}; std::vector<int32_t> fired;
      switch(action) {
        case 0: result = controls.command(static_cast<uint16_t>(args.at(0))); break;
        case 1: controls.select(static_cast<size_t>(args.at(0))); break;
        case 2: controls.toggle_inclusion(static_cast<size_t>(args.at(0))); break;
        case 3: controls.toggle_pair(); break;
        case 4: controls.change_group(static_cast<uint8_t>(args.at(0))); break;
        case 5: {
          controls.hold_selection(args.at(1) != 0); controls.set_manual_guidance(args.at(2) != 0);
          std::array<bool, 5> locks; for(size_t i{0}; i != locks.size(); ++i) locks[i] = args.at(3+i) != 0;
          controls.update_selection(args.at(0), locks); break;
        }
        case 6: controls.fast_charge(); break;
        case 7: result = bank.allocate(controls.selection(), static_cast<int16_t>(args.at(1)), static_cast<int16_t>(args.at(0)), args.at(2) != 0); break;
        case 8: release = controls.fire(args.at(0) != 0, [&](size_t slot){ fired.push_back(static_cast<int32_t>(slot)); actor.discharge_aimed_slot(slot, args.at(1) != 0); }); break;
        case 9: bank.at(static_cast<size_t>(args.at(0))).controls().disabled = args.at(1) != 0; break;
        case 10: {
          auto &weapon{bank.at(static_cast<size_t>(args.at(0)))};
          if(auto *energy{weapon.energy()}) energy->charge = static_cast<int16_t>(args.at(1));
          else if(auto *ammo{weapon.ammunition()}) ammo->rounds = static_cast<int16_t>(args.at(1));
          break;
        }
        case 11: controls.set_tracking(true); break;
        case 12:
          release = controls.fire(args.at(0) != 0, [&](size_t slot){ fired.push_back(static_cast<int32_t>(slot)); actor.discharge_aimed_slot(slot, args.at(1) != 0); });
          result = bank.allocate(controls.selection(), static_cast<int16_t>(args.at(3)), static_cast<int16_t>(args.at(2)), args.at(1) != 0); break;
        default: file.fail("unknown player weapon action");
      }
      auto const where{name + '/' + std::to_string(row) + '/' + std::to_string(step)};
      if(result != expected_result || release != expected_release || fired != expected_fired) file.fail(where + " control result/discharge differs");
      check(static_cast<int>(step)); ++updates; discharges += static_cast<unsigned int>(fired.size());
    }
  }
  file.require_end();
  if(count != 34 || updates != 11594) file.fail("original player weapon coverage differs");
  std::cout << count << " original player weapon banks, " << updates << " control updates and " << discharges << " discharges agree\n";
  return EXIT_SUCCESS;
} catch(std::exception const &error) { std::cerr << error.what() << '\n'; return EXIT_FAILURE; }
