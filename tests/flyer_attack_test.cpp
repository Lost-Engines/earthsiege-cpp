#include <array>
#include <cstdlib>
#include <iostream>
#include <string>
#include "resources/resource_store.h"
#include "simulation/flyer_attack.h"
#include "simulation/projectile_instance.h"

namespace {
using namespace earthsiege;
flyer_motion read_state(binary_reader &file) {
  return {
    {file.integer<int32_t>(),file.integer<int32_t>(),file.integer<int32_t>()},
    {file.integer<uint16_t>(),file.integer<uint16_t>(),file.integer<uint16_t>()},
    {file.integer<int16_t>(),file.integer<int16_t>(),file.integer<int16_t>()},
    {file.integer<int16_t>(),file.integer<int16_t>(),file.integer<int16_t>()},
    file.integer<int16_t>(),file.integer<int16_t>(),file.integer<uint8_t>() != 0,
  };
}
void compare(flyer_motion const &a, flyer_motion const &e, std::string const &label) {
  if(a.position != e.position || a.angles != e.angles || a.velocity != e.velocity || a.angular_step != e.angular_step || a.obstacle_strength != e.obstacle_strength || a.obstacle_timer != e.obstacle_timer || a.obstacle != e.obstacle) {
    std::cerr << label << " actual position " << a.position << " angles " << a.angles << " rates " << a.angular_step << " strength/timer " << a.obstacle_strength << '/' << a.obstacle_timer << '\n';
    std::cerr << label << " expected position " << e.position << " angles " << e.angles << " rates " << e.angular_step << " strength/timer " << e.obstacle_strength << '/' << e.obstacle_timer << '\n';
    throw std::runtime_error{label + " flight state"};
  }
}
}
namespace {
flyer_attack_state read_attack(binary_reader &file) {
  return {static_cast<flyer_attack_phase>(file.integer<int16_t>()),file.integer<int16_t>(),file.integer<int16_t>(),file.integer<int16_t>(),file.integer<int32_t>(),file.integer<uint8_t>() != 0,file.integer<uint8_t>()};
}
random_state::state read_random(binary_reader &file) {
  random_state::state result; for(auto &v : result.words) v = file.integer<uint16_t>(); result.first = file.integer<uint8_t>(); result.second = file.integer<uint8_t>(); return result;
}
}
int main(int const argc, char **argv) try {
  if(argc != 3) return EXIT_FAILURE;
  resource_store resources{argv[1]}; auto const definition{read_flyer_definition(resources,"SKIMMER")};
  auto const weapons{read_weapon_catalogue(resources.read("simvol0","dat/WEAPONS.DAT"))};
  auto const bytes{read_file(argv[2])}; binary_reader file{bytes}; auto const magic{file.take(8)};
  if(std::string(magic.begin(),magic.end()) != "ESFATK01") file.fail("fixture version");
  auto const paths{file.integer<uint16_t>()}; unsigned int steps{0},shots{0};
  for(unsigned int row{0}; row != paths; ++row) {
    auto flight{read_state(file)}; auto attack{read_attack(file)}; random_state random{read_random(file)}; auto const count{file.integer<uint16_t>()};
    for(unsigned int step{0}; step != count; ++step) {
      flyer_attack_target const target{{file.integer<int32_t>(),file.integer<int32_t>(),file.integer<int32_t>()},file.integer<uint16_t>(),file.integer<int16_t>()};
      auto const ratio{file.integer<int16_t>()}; auto const expected_flight{read_state(file)}; auto const expected_attack{read_attack(file)}; auto const expected_random{read_random(file)};
      std::vector<std::array<int32_t,9>> expected_shots(file.integer<uint16_t>()), actual;
      for(auto &shot : expected_shots) for(auto &v : shot) v = file.integer<int32_t>();
      attack_with_flyer(flight,attack,definition.profile,target,weapons,ratio,random,[&](weapon_shot const &shot){
        auto const &p{weapons.projectiles.at(shot.projectile)};
        actual.push_back({static_cast<int32_t>(p.kind),p.subtype,shot.inherited_speed,shot.muzzle.position.x,shot.muzzle.position.y,shot.muzzle.position.z,shot.muzzle.angles.x,shot.muzzle.angles.y,shot.muzzle.angles.z});
      });
      auto const label{std::to_string(row) + '/' + std::to_string(step)};
      compare(flight,expected_flight,label);
      if(attack.phase != expected_attack.phase || attack.requested_speed != expected_attack.requested_speed || attack.missile_latch != expected_attack.missile_latch || attack.refire_timer != expected_attack.refire_timer || attack.manoeuvre_timer != expected_attack.manoeuvre_timer || attack.firing_window != expected_attack.firing_window || attack.contact_state != expected_attack.contact_state) file.fail(label + " attack state");
      if(actual != expected_shots) file.fail(label + " projectile requests");
      auto const &rng{random.snapshot()}; if(rng.words != expected_random.words || rng.first != expected_random.first || rng.second != expected_random.second) file.fail(label + " RNG");
      ++steps; shots += static_cast<unsigned int>(actual.size());
    }
  }
  projectile_motions const motions{read_bullet_motions(resources.read("simvol0","dat/BULLETS.DAT")),read_rocket_motions(resources.read("simvol0","dat/ROCKETS.DAT"))};
  auto const bullets{read_model(resources.read("simvol0","dts/BULLETS.DTS"))}; auto const rockets{read_model(resources.read("simvol0","dts/ROCKETS.DTS"))};
  auto const rocket{std::ranges::find_if(weapons.projectiles,[](projectile_definition const &p){ return p.kind == projectile_kind::rocket && p.subtype == 0; })};
  auto const launches{file.integer<uint16_t>()};
  for(unsigned int i{0}; i != launches; ++i) {
    weapon_shot shot; shot.kind = shot_kind::rocket; shot.projectile = static_cast<size_t>(rocket-weapons.projectiles.begin()); shot.inherited_speed = file.integer<int16_t>(); auto const expected{file.integer<int16_t>()};
    projectile_instance projectile{shot,weapons,motions,bullets,rockets}; if(projectile.flight().speed != expected) file.fail("inherited rocket speed");
  }
  file.require_end(); if(paths != 192 || steps != 3072 || shots != 78 || launches != 6) file.fail("flyer attack coverage");
  std::cout << steps << " original attack updates, " << shots << " projectile requests and " << launches << " rocket launch speeds agree\n"; return EXIT_SUCCESS;
} catch(std::exception const &error) { std::cerr << error.what() << '\n'; return EXIT_FAILURE; }
