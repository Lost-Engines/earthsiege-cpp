#include <cstdlib>
#include <iostream>
#include <string>
#include "resources/resource_store.h"
#include "simulation/ground_actor.h"

int main(int const argc, char **argv) try {
  if(argc != 3) return EXIT_FAILURE;
  earthsiege::resource_store resources{argv[1]}; earthsiege::ground_catalogue catalogue{resources};
  auto const weapons{earthsiege::read_weapon_catalogue(resources.read("simvol0","dat/WEAPONS.DAT"))};
  auto const bytes{earthsiege::read_file(argv[2])}; earthsiege::binary_reader file{bytes}; auto const magic{file.take(8)};
  if(std::string(magic.begin(),magic.end()) != "ESTFIRE1") file.fail("fixture version");
  unsigned int steps{0}, shots{0}; auto const count{file.integer<uint16_t>()};
  auto const projectile{file.integer<int16_t>()}; if(weapons.projectiles.at(2).speed != file.integer<int16_t>()) file.fail("lead speed");
  for(unsigned int row{0}; row != count; ++row) {
    vec3<int32_t> const position{file.integer<int32_t>(),file.integer<int32_t>(),file.integer<int32_t>()}; auto const heading{file.integer<uint16_t>()};
    earthsiege::ground_actor actor{catalogue,8,1,100,position,{0,0,heading}};
    auto const amount{file.integer<uint16_t>()};
    for(unsigned int step{0}; step != amount; ++step) {
      auto const ratio{file.integer<int16_t>()}; bool const available{file.integer<uint8_t>() != 0};
      earthsiege::turret_target input;
      input.position = vec3<int32_t>{file.integer<int32_t>(),file.integer<int32_t>(),file.integer<int32_t>()};
      input.offset = vec3<int16_t>{file.integer<int16_t>(),file.integer<int16_t>(),file.integer<int16_t>()};
      input.angles = vec3<uint16_t>{file.integer<uint16_t>(),file.integer<uint16_t>(),file.integer<uint16_t>()}; input.speed = file.integer<int16_t>();
      using event = std::array<int32_t,8>; std::vector<event> actual;
      earthsiege::turret_callbacks callbacks;
      callbacks.select = [&]() -> std::optional<size_t> { actual.push_back({0,available,0,0,0,0,0,0}); return available ? std::optional<size_t>{0} : std::nullopt; };
      callbacks.target = [&](size_t id){ if(id != 0) file.fail("target identity"); return earthsiege::turret_target{input}; };
      callbacks.fire = [&](earthsiege::weapon_shot const &shot){
        if(shot.kind != earthsiege::shot_kind::bullet || shot.projectile != static_cast<size_t>(projectile) || shot.charge != -1) file.fail("turret projectile definition");
        auto const &p{shot.muzzle.position}; auto const &a{shot.muzzle.angles};
        actual.push_back({1,2,a.x,a.y,a.z,p.x,p.y,p.z}); ++shots;
      };
      actor.advance_turret(ratio,weapons,callbacks);
      auto const label{std::to_string(row) + '/' + std::to_string(step)}; auto const &f{actor.turret_fire()};
      if(f.target.has_value() != (file.integer<uint8_t>() != 0)) file.fail(label + " target");
      for(int16_t value : {f.target_timer,f.burst_timer,static_cast<int16_t>(f.burst_phase),f.refire_timer,actor.cell_clock()}) if(value != file.integer<int16_t>()) file.fail(label + " timer");
      if(actor.instance().cell_frames()[0] != file.integer<uint16_t>()) file.fail(label + " cells");
      for(auto value : actor.turret_aim().rates) if(value != file.integer<int16_t>()) file.fail(label + " rate");
      for(auto value : actor.turret_aim().angles) if(value != file.integer<int16_t>()) file.fail(label + " angle");
      for(auto const &matrix : actor.instance().transforms()) {
        for(unsigned int const field : {0U,1U,3U,4U,2U,5U,6U,7U,8U}) if(matrix.basis.data[field] != file.integer<int16_t>()) file.fail(label + " basis");
        if(matrix.type != file.integer<uint8_t>()) file.fail(label + " matrix type");
        file.integer<uint8_t>();
        for(unsigned int axis{0}; axis != 3; ++axis) if(matrix.translation[axis] != file.integer<int32_t>()) file.fail(label + " translation");
      }
      std::vector<event> expected(file.integer<uint16_t>()); for(auto &e : expected) for(auto &v : e) v = file.integer<int32_t>();
      if(actual != expected) file.fail(label + " events");
      ++steps;
    }
  }
  file.require_end(); if(count != 32 || steps != 3072 || shots < 20) file.fail("firing coverage differs");
  std::cout << steps << " original turret firing updates and " << shots << " muzzle requests agree\n";
  return EXIT_SUCCESS;
} catch(std::exception const &error) { std::cerr << error.what() << '\n'; return EXIT_FAILURE; }
