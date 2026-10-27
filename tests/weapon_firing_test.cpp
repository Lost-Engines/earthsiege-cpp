#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include "resources/resource_store.h"
#include "simulation/herc_actor.h"
#include "simulation/weapon_firing.h"

int main(int const argc, char **argv) try {
  if(argc != 3) return EXIT_FAILURE;
  earthsiege::resource_store resources{argv[1]};
  auto const definitions{earthsiege::read_weapon_catalogue(resources.read("simvol0", "dat/WEAPONS.DAT"))};
  auto const templates{earthsiege::read_model(resources.read("simvol0", "dts/MECHWPNS.DTS"))};
  auto const bytes{earthsiege::read_file(argv[2])};
  earthsiege::binary_reader file{bytes};
  auto const magic{file.take(8)};
  if(std::string(magic.begin(), magic.end()) != "ESFIRE01") file.fail("fixture version");
  auto const count{file.integer<uint32_t>()};
  unsigned int shots_total{0};
  auto const shorts{[&]{ std::vector<int16_t> values(file.integer<uint16_t>()); for(auto &value : values) value = file.integer<int16_t>(); return values; }};
  for(unsigned int row{0}; row != count; ++row) {
    auto const name{file.cstring()};
    auto const equipment{shorts()};
    auto const values{shorts()};
    auto const weapon_id{file.integer<int16_t>()};
    auto const slot{static_cast<size_t>(file.integer<int16_t>())};
    auto const amount{file.integer<int16_t>()};
    bool const free{file.integer<uint8_t>() != 0};
    vec3<uint16_t> const angles{file.integer<uint16_t>(), file.integer<uint16_t>(), file.integer<uint16_t>()};
    vec3<int32_t> const position{file.integer<int32_t>(), file.integer<int32_t>(), file.integer<int32_t>()};
    vec2<int16_t> const aim{file.integer<int16_t>(), file.integer<int16_t>()};
    std::array<int16_t, 7> expected_state;
    for(auto &value : expected_state) value = file.integer<int16_t>();
    auto const definition{earthsiege::read_herc_definition(resources, name)};
    earthsiege::herc_actor actor{definition, definitions, templates, equipment, position, angles, values};
    auto &weapon{actor.weapons().at(slot)};
    auto *energy{weapon.energy()};
    auto *ammo{weapon.ammunition()};
    auto *grenade{std::get_if<earthsiege::grenade_weapon>(&weapon.mechanism)};
    if(energy) energy->charge = amount;
    if(ammo) ammo->rounds = amount;
    if(grenade) { grenade->holding = true; grenade->throw_range = 20000; }
    auto const shots{actor.discharge_slot(slot, aim, free)};
    auto const &control{weapon.controls()};
    std::array<int16_t, 7> const state{control.refire_remaining, control.hud_changes, control.refreshed_changes, control.firing_animation, ammo ? ammo->available : weapon_id != 13, grenade ? grenade->holding : false, energy ? energy->charge : ammo ? ammo->rounds : amount};
    auto const require{[&](bool const correct, std::string const &field){ if(!correct) file.fail("discharge " + std::to_string(row) + ' ' + name + '/' + std::to_string(weapon_id) + '/' + std::to_string(slot) + ": " + field); }};
    for(size_t i{0}; i != state.size(); ++i) require(state[i] == expected_state[i], "state " + std::to_string(i) + ": " + std::to_string(state[i]) + " != " + std::to_string(expected_state[i]));
    auto const shot_count{file.integer<uint16_t>()};
    require(shot_count == shots.size(), "spawn count");
    for(auto const &shot : shots) {
      auto const kind{file.integer<uint8_t>()};
      auto const charge{file.integer<int16_t>()};
      auto const range{file.integer<int32_t>()};
      auto const subtype{file.integer<int16_t>()};
      vec3<int32_t> const shot_position{file.integer<int32_t>(), file.integer<int32_t>(), file.integer<int32_t>()};
      vec3<uint16_t> const shot_angles{file.integer<uint16_t>(), file.integer<uint16_t>(), file.integer<uint16_t>()};
      require(std::to_underlying(shot.kind) == kind && shot.charge == charge && shot.range == range && definitions.projectiles[shot.projectile].subtype == subtype, "spawn type/power");
      require(shot.muzzle.position == shot_position, "muzzle position");
      require(shot.muzzle.angles == shot_angles, "muzzle angles");
      for(unsigned int const field : {0U, 1U, 3U, 4U, 2U, 5U, 6U, 7U, 8U}) require(shot.muzzle.transform.basis.data[field] == file.integer<int16_t>(), "muzzle basis");
      require(shot.muzzle.transform.type == file.integer<uint8_t>(), "muzzle transform type");
      file.integer<uint8_t>();
      for(unsigned int axis{0}; axis != 3; ++axis) require(shot.muzzle.transform.translation[axis] == file.integer<int32_t>(), "muzzle transform translation");
      ++shots_total;
    }
  }
  file.require_end();
  if(count != 1160 || shots_total != 1272) file.fail("firing reference coverage differs");
  std::cout << count << " original weapon discharges and " << shots_total << " projectile creation calls agree" << std::endl;
  return EXIT_SUCCESS;
} catch(std::exception const &error) {
  std::cerr << error.what() << std::endl;
  return EXIT_FAILURE;
}
