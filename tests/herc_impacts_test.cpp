#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include "resources/resource_store.h"
#include "simulation/herc_actor.h"
#include "simulation/model_math.h"

namespace {

vec3<int32_t> point(earthsiege::binary_reader &file) { return {file.integer<int32_t>(), file.integer<int32_t>(), file.integer<int32_t>()}; }

struct state {
  earthsiege::shield_power shields;
  earthsiege::herc_impact_state impacts;
};

state read_state(earthsiege::binary_reader &file) {
  state value;
  value.shields = {file.integer<int16_t>(), file.integer<int16_t>(), file.integer<int16_t>(), file.integer<int16_t>()};
  value.impacts = {file.integer<uint8_t>() != 0, file.integer<uint8_t>() != 0, file.integer<int16_t>(), file.integer<int32_t>()};
  return value;
}

} // namespace

int main(int const argc, char **argv) try {
  if(argc != 3) return EXIT_FAILURE;
  earthsiege::resource_store resources{argv[1]};
  auto const weapons{earthsiege::read_weapon_catalogue(resources.read("simvol0", "dat/WEAPONS.DAT"))};
  auto const templates{earthsiege::read_model(resources.read("simvol0", "dts/MECHWPNS.DTS"))};
  auto const bytes{earthsiege::read_file(argv[2])};
  earthsiege::binary_reader file{bytes};
  auto const magic{file.take(8)};
  if(std::string(magic.begin(), magic.end()) != "ESHERC01") file.fail("fixture version");
  auto const chassis{file.integer<uint16_t>()};
  unsigned int total{0}, hits{0}, body_hits{0};
  for(unsigned int field{0}; field != chassis; ++field) {
    auto const name{file.cstring()};
    std::vector<int16_t> equipment(file.integer<uint16_t>());
    for(auto &weapon : equipment) weapon = file.integer<int16_t>();
    auto const definition{earthsiege::read_herc_definition(resources, name)};
    auto const shield_height{file.integer<int16_t>()}; auto const shield_radius{file.integer<int16_t>()};
    if(definition.profile.shield_height != shield_height || definition.profile.shield_radius != shield_radius) file.fail(name + " shield geometry fields");
    auto const count{file.integer<uint16_t>()};
    for(unsigned int row{0}; row != count; ++row) {
      auto const label{name + '/' + std::to_string(row)};
      auto const origin{point(file)}; auto const heading{file.integer<uint16_t>()};
      earthsiege::impact_request request;
      request.transform = earthsiege::rotation_transform({0, 0, heading}); request.transform.translation = origin;
      request.reach = file.integer<int32_t>(); request.radius = file.integer<int16_t>();
      request.damage.armour = file.integer<int16_t>(); request.damage.shield = file.integer<int16_t>(); request.damage.splash_q10 = file.integer<int16_t>();
      auto const signature{file.integer<int16_t>()};
      if(signature != 5) request.damage.missile_subtype = static_cast<uint8_t>(signature);
      earthsiege::impact_conditions conditions;
      conditions.player = file.integer<uint8_t>() != 0; conditions.attacker_side = file.integer<uint8_t>(); conditions.difficulty = file.integer<int16_t>(); conditions.difficulty_override = file.integer<uint8_t>() != 0;
      auto const initial{read_state(file)}; auto const expected{read_state(file)};
      auto const expected_distance{file.integer<int32_t>()};
      std::optional<earthsiege::body_damage_request> expected_body;
      if(file.integer<uint8_t>()) expected_body = earthsiege::body_damage_request{file.integer<uint16_t>(), file.integer<int16_t>(), file.integer<int16_t>()};
      bool const has_position{file.integer<uint8_t>() != 0}; auto const warnings{file.integer<uint8_t>()};
      std::optional<vec3<int32_t>> position;
      if(has_position) position = point(file);
      earthsiege::herc_actor actor{definition, weapons, templates, equipment, {0, 10000, 0}, {0, 0, 0}};
      actor.power().shields = initial.shields; actor.impacts() = initial.impacts;
      unsigned int warning_count{0};
      auto const result{actor.intercept(request, conditions, [&](){ ++warning_count; })};
      if((result ? result->distance : 0) != expected_distance) file.fail(label + " interception depth");
      if(static_cast<bool>(result && result->body) != expected_body.has_value()) file.fail(label + " body-damage gate");
      if(expected_body && (result->body->section != expected_body->section || result->body->direct != expected_body->direct || result->body->radial != expected_body->radial)) file.fail(label + " body-damage request");
      if(position && (!result || result->position != *position)) file.fail(label + " impact position");
      if(warning_count != warnings) file.fail(label + " shield warning");
      auto const &shield{actor.power().shields}; auto const &state{actor.impacts()};
      if(shield.front != expected.shields.front || shield.rear != expected.shields.rear || shield.balance != expected.shields.balance || shield.capacity != expected.shields.capacity) file.fail(label + " shield absorption");
      if(state.detectable != expected.impacts.detectable || state.ecm_remaining != expected.impacts.ecm_remaining || state.shield_warning != expected.impacts.shield_warning || state.damage_received != expected.impacts.damage_received) file.fail(label + " impact state");
      ++total; hits += result.has_value(); body_hits += expected_body.has_value();
    }
  }
  file.require_end();
  if(chassis != 8 || total != 1152 || hits != 445 || body_hits != 178) file.fail("HERC interception reference coverage differs");
  std::cout << total << " original HERC interceptions agree (" << hits << " hits, " << body_hits << " body-damage requests)" << std::endl;
  return EXIT_SUCCESS;
} catch(std::exception const &error) {
  std::cerr << error.what() << std::endl;
  return EXIT_FAILURE;
}
