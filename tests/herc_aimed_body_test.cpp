#include <cstdlib>
#include <iostream>
#include "herc_body_reference.h"
#include "resources/resource_store.h"
#include "simulation/herc_actor.h"

int main(int argc, char **argv) try {
  using namespace earthsiege;
  if(argc != 3) return EXIT_FAILURE;
  resource_store resources{argv[1]};
  auto const definitions{read_weapon_catalogue(resources.read("simvol0", "dat/WEAPONS.DAT"))};
  auto const templates{read_model(resources.read("simvol0", "dts/MECHWPNS.DTS"))};
  auto const bytes{read_file(argv[2])}; binary_reader file{bytes}; auto const magic{file.take(8)};
  if(std::string(magic.begin(), magic.end()) != "ESHAB001") file.fail("fixture version");
  auto const words{[&]{ std::vector<int16_t> out(file.integer<uint16_t>()); for(auto &v : out) v = file.integer<int16_t>(); return out; }};
  auto const actors{file.integer<uint16_t>()}; size_t frames{0}, shots_total{0};
  for(unsigned int i{0}; i != actors; ++i) {
    auto const name{file.cstring()}; auto const equipment{words()}, values{words()};
    auto const initial{herc_reference::read_state(file)};
    auto const definition{read_herc_definition(resources, name)};
    herc_actor actor{definition, definitions, templates, equipment, initial.position, initial.angles, values};
    herc_reference::check(actor.body(), initial, name + "/initial");
    auto const count{file.integer<uint16_t>()};
    for(unsigned int frame{0}; frame != count; ++frame) {
      auto const label{name + '/' + std::to_string(frame)};
      auto const ratio{file.integer<int16_t>()}, speed{file.integer<int16_t>()}, turn{file.integer<int16_t>()};
      bool const centre{file.integer<uint8_t>() != 0}; auto const distance{file.integer<int32_t>()};
      vec3<int32_t> const point{file.integer<int32_t>(), file.integer<int32_t>(), file.integer<int32_t>()};
      vec3<int16_t> const errors{file.integer<int16_t>(), file.integer<int16_t>(), file.integer<int16_t>()};
      auto const slot{file.integer<int16_t>()};
      actor.begin_frame(); actor.advance(ratio); actor.prepare_requested(speed, turn, ratio);
      if(centre) actor.centre_aim(distance, ratio);
      else if(actor.aim_at(point, ratio) != errors) file.fail(label + " articulated aim errors");
      auto const slots{file.integer<uint16_t>()}; if(slots != actor.weapons().slots().size()) file.fail(label + " bank slots");
      for(unsigned int w{0}; w != slots; ++w) {
        bool const present{file.integer<uint8_t>() != 0}; if(present != actor.weapons().slots()[w].has_value()) file.fail(label + " weapon presence");
        if(present) {
          vec3<int16_t> const expected{file.integer<int16_t>(), file.integer<int16_t>(), file.integer<int16_t>()};
          if(actor.weapons().at(w).convergence != expected) file.fail(label + " weapon convergence");
        }
      }
      std::vector<weapon_shot> shots;
      if(slot >= 0) {
        auto &weapon{actor.weapons().at(static_cast<size_t>(slot))};
        if(auto energy{weapon.energy()}) energy->charge = 1200;
        if(auto ammunition{weapon.ammunition()}) ammunition->rounds = 100;
        shots = actor.discharge_aimed_slot(static_cast<size_t>(slot));
      }
      if(shots.size() != file.integer<uint16_t>()) file.fail(label + " shot count");
      for(auto const &shot : shots) {
        auto const kind{file.integer<uint8_t>()}; auto const charge{file.integer<int16_t>()}; auto const range{file.integer<int32_t>()}; auto const subtype{file.integer<int16_t>()};
        vec3<int32_t> const position{file.integer<int32_t>(), file.integer<int32_t>(), file.integer<int32_t>()};
        vec3<uint16_t> const angles{file.integer<uint16_t>(), file.integer<uint16_t>(), file.integer<uint16_t>()};
        if(std::to_underlying(shot.kind) != kind || shot.charge != charge || shot.range != range || definitions.projectiles[shot.projectile].subtype != subtype) file.fail(label + " shot definition");
        if(shot.muzzle.position != position || shot.muzzle.angles != angles) file.fail(label + " muzzle position/orientation");
        for(unsigned int const field : {0U, 1U, 3U, 4U, 2U, 5U, 6U, 7U, 8U}) if(shot.muzzle.transform.basis.data[field] != file.integer<int16_t>()) file.fail(label + " muzzle basis");
        if(shot.muzzle.transform.type != file.integer<uint8_t>()) file.fail(label + " muzzle transform type");
        file.integer<uint8_t>();
        for(unsigned int axis{0}; axis != 3; ++axis) if(shot.muzzle.transform.translation[axis] != file.integer<int32_t>()) file.fail(label + " muzzle translation");
        ++shots_total;
      }
      herc_reference::check(actor.body(), herc_reference::read_state(file), label);
      ++frames;
    }
  }
  file.require_end(); if(actors != 8 || frames != 512 || shots_total == 0) file.fail("composed aiming coverage");
  std::cout << frames << " composed original HERC frames and " << shots_total << " aimed projectiles agree\n";
  return EXIT_SUCCESS;
} catch(std::exception const &e) { std::cerr << e.what() << '\n'; return EXIT_FAILURE; }
