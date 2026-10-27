#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include "resources/resource_store.h"
#include "simulation/effect_state.h"
#include "simulation/herc_actor.h"
#include "simulation/model_math.h"

namespace {

vec3<int32_t> point(earthsiege::binary_reader &file) { return {file.integer<int32_t>(), file.integer<int32_t>(), file.integer<int32_t>()}; }
std::vector<int16_t> shorts(earthsiege::binary_reader &file) {
  std::vector<int16_t> result(file.integer<uint16_t>());
  for(auto &v : result) v = file.integer<int16_t>();
  return result;
}
struct state {
  earthsiege::shield_power shields;
  earthsiege::damage_state damage;
  earthsiege::herc_impact_state impacts;
  earthsiege::random_state::state random;
};
state read_state(earthsiege::binary_reader &file) {
  state value;
  value.shields = {file.integer<int16_t>(), file.integer<int16_t>(), file.integer<int16_t>(), file.integer<int16_t>()};
  value.damage.consequences = {file.integer<uint8_t>() != 0, file.integer<uint8_t>() != 0, file.integer<uint8_t>() != 0, file.integer<uint8_t>() != 0, file.integer<uint8_t>() != 0, file.integer<uint8_t>() != 0};
  value.impacts = {file.integer<uint8_t>() != 0, file.integer<uint8_t>() != 0, file.integer<int16_t>(), file.integer<int32_t>()};
  for(auto &v : value.random.words) v = file.integer<uint16_t>();
  value.random.first = file.integer<uint8_t>(); value.random.second = file.integer<uint8_t>();
  value.damage.internal = shorts(file); value.damage.armour = shorts(file); value.damage.present = shorts(file); value.damage.cells = shorts(file);
  return value;
}

} // namespace

int main(int const argc, char **argv) try {
  if(argc != 3) return EXIT_FAILURE;
  earthsiege::resource_store resources{argv[1]};
  auto const weapons{earthsiege::read_weapon_catalogue(resources.read("simvol0", "dat/WEAPONS.DAT"))};
  auto const templates{earthsiege::read_model(resources.read("simvol0", "dts/MECHWPNS.DTS"))};
  auto const explosions{earthsiege::read_explosions(resources.read("simvol0", "dat/EXPLOS.DAT"))};
  auto const debris{earthsiege::read_debris(resources.read("simvol0", "dat/DEBRIS.DAT"))};
  auto const effects{earthsiege::read_model(resources.read("simvol0", "dts/EXPLOS.DTS"))};
  auto const bytes{earthsiege::read_file(argv[2])}; earthsiege::binary_reader file{bytes};
  auto const magic{file.take(8)};
  if(std::string(magic.begin(), magic.end()) != "ESHCOM01") file.fail("fixture version");
  auto const chassis{file.integer<uint16_t>()};
  unsigned int total{0}, hits{0}, event_count{0};
  for(unsigned int field{0}; field != chassis; ++field) {
    auto const name{file.cstring()}; auto const equipment{shorts(file)};
    auto const definition{earthsiege::read_herc_definition(resources, name)};
    earthsiege::impact_payload payload;
    for(auto &v : payload.shield_effects) v = file.integer<int16_t>();
    for(auto &v : payload.ground_effects) v = file.integer<int16_t>();
    for(auto &v : payload.armour_effects) v = file.integer<int16_t>();
    auto const count{file.integer<uint16_t>()};
    for(unsigned int row{0}; row != count; ++row) {
      auto const label{name + '/' + std::to_string(row)};
      auto const origin{point(file)}; auto const heading{file.integer<uint16_t>()};
      earthsiege::impact_request request;
      request.transform = earthsiege::rotation_transform({0, 0, heading}); request.transform.translation = origin;
      request.reach = file.integer<int32_t>(); request.radius = file.integer<int16_t>(); request.damage = payload;
      request.damage.armour = file.integer<int16_t>(); request.damage.shield = file.integer<int16_t>(); request.damage.splash_q10 = file.integer<int16_t>();
      auto const signature{file.integer<int16_t>()};
      if(signature != 5) request.damage.missile_subtype = static_cast<uint8_t>(signature);
      earthsiege::impact_conditions conditions;
      conditions.player = file.integer<uint8_t>() != 0; conditions.attacker_side = file.integer<uint8_t>(); conditions.difficulty = file.integer<int16_t>(); conditions.difficulty_override = file.integer<uint8_t>() != 0; conditions.side = file.integer<uint8_t>(); conditions.invulnerable = file.integer<uint8_t>() != 0;
      auto const initial{read_state(file)}; auto const expected{read_state(file)};
      auto const expected_distance{file.integer<int32_t>()};
      std::vector<std::vector<int32_t>> expected_events(file.integer<uint16_t>());
      for(auto &event : expected_events) { event.resize(file.integer<uint8_t>()); for(auto &v : event) v = file.integer<int32_t>(); }
      earthsiege::herc_actor actor{definition, weapons, templates, equipment, {0, 10000, 0}, {0, 0, 0}};
      actor.power().shields = initial.shields; actor.impacts() = initial.impacts;
      auto const check_damage{[&](earthsiege::damage_state const &target){
        auto const &actual{actor.damage()};
        if(actual.internal != target.internal) file.fail(label + " internal damage");
        if(actual.armour != target.armour) file.fail(label + " armour damage");
        if(actual.present != target.present) file.fail(label + " section presence");
        if(actual.cells != target.cells) file.fail(label + " damage animation cells");
        if(actual.consequences != target.consequences) file.fail(label + " damage consequences");
      }};
      check_damage(initial.damage);
      earthsiege::random_state random{initial.random};
      std::vector<std::vector<int32_t>> events;
      earthsiege::herc_damage_events sinks;
      sinks.explosion = [&](int16_t const id, vec3<int32_t> const &position, earthsiege::random_state &rng){
        events.push_back({1, id, position.x, position.y, position.z});
        auto const &explosion{explosions.at(static_cast<size_t>(id))};
        auto const &shape{std::get<earthsiege::shape_part>(effects.roots.at(static_cast<size_t>(explosion.model)).content)};
        earthsiege::create_explosion(explosion, shape.cell_frame_counts.size(), position, rng);
      };
      sinks.debris = [&](int16_t const group, vec3<int32_t> const &position, earthsiege::random_state &rng){
        events.push_back({2, group, position.x, position.y, position.z});
        earthsiege::create_debris_burst(debris.at(static_cast<size_t>(group)), position, rng);
      };
      sinks.shield_warning = [&](){ events.push_back({3}); };
      sinks.cockpit_damage = [&](size_t const section){ events.push_back({4, static_cast<int32_t>(section)}); };
      sinks.attached_smoke = [&](int16_t const part, vec3<int16_t> const &position){ events.push_back({5, part, position.x, position.y, position.z}); };
      sinks.body_damage = [&](size_t const section, int16_t const amount){ events.push_back({0, amount, static_cast<int32_t>(section)}); };
      sinks.weapon_ejected = [&](size_t const slot){ events.push_back({6, definition.mounts.at(slot).equipment_slot}); };
      sinks.salvage = [&](int16_t const id, int16_t const quality){ events.push_back({7, id, quality}); };
      auto const result{actor.receive_impact(request, conditions, random, sinks)};
      if((result ? result->distance : 0) != expected_distance) file.fail(label + " interception depth");
      if(events != expected_events) {
        for(auto const &e : events) { std::cerr << "actual:"; for(auto v : e) std::cerr << ' ' << v; std::cerr << '\n'; }
        for(auto const &e : expected_events) { std::cerr << "expected:"; for(auto v : e) std::cerr << ' ' << v; std::cerr << '\n'; }
        file.fail(label + " effect/event order");
      }
      check_damage(expected.damage);
      auto const &shield{actor.power().shields}; auto const &impact{actor.impacts()}; auto const &rng{random.snapshot()};
      if(shield.front != expected.shields.front || shield.rear != expected.shields.rear || shield.balance != expected.shields.balance || shield.capacity != expected.shields.capacity) file.fail(label + " shield absorption");
      if(impact.detectable != expected.impacts.detectable || impact.ecm_remaining != expected.impacts.ecm_remaining || impact.shield_warning != expected.impacts.shield_warning || impact.damage_received != expected.impacts.damage_received) file.fail(label + " impact state");
      if(rng.words != expected.random.words || rng.first != expected.random.first || rng.second != expected.random.second) file.fail(label + " final random state");
      ++total; hits += result.has_value(); event_count += static_cast<unsigned int>(events.size());
    }
  }
  file.require_end();
  if(chassis != 8 || total != 2304 || hits != 1597) file.fail("HERC damage reference coverage differs");
  std::cout << total << " original HERC impacts agree, including component state, " << event_count << " ordered effects/notifications and shared randomness" << std::endl;
  return EXIT_SUCCESS;
} catch(std::exception const &error) {
  std::cerr << error.what() << std::endl;
  return EXIT_FAILURE;
}
