#include <cstdlib>
#include <iostream>
#include <string>
#include "resources/resource_store.h"
#include "simulation/effect_state.h"
#include "simulation/herc_actor.h"

namespace {

std::vector<int16_t> shorts(earthsiege::binary_reader &file) {
  std::vector<int16_t> result(file.integer<uint16_t>());
  for(auto &v : result) v = file.integer<int16_t>();
  return result;
}
struct state {
  earthsiege::damage_state damage;
  std::vector<int16_t> weapons;
  bool salvage_excluded, collapsed, detectable;
  earthsiege::random_state::state random;
};
state read_state(earthsiege::binary_reader &file) {
  state s;
  s.damage.internal = shorts(file); s.damage.armour = shorts(file); s.damage.present = shorts(file); s.damage.cells = shorts(file); s.weapons = shorts(file);
  s.damage.consequences = {file.integer<uint8_t>() != 0, file.integer<uint8_t>() != 0, file.integer<uint8_t>() != 0, file.integer<uint8_t>() != 0, file.integer<uint8_t>() != 0, file.integer<uint8_t>() != 0};
  s.salvage_excluded = file.integer<uint8_t>() != 0; s.collapsed = file.integer<uint8_t>() != 0; s.detectable = file.integer<uint8_t>() != 0;
  for(auto &v : s.random.words) v = file.integer<uint16_t>();
  s.random.first = file.integer<uint8_t>(); s.random.second = file.integer<uint8_t>();
  return s;
}
void compare_damage(earthsiege::binary_reader &file, std::string const &label, earthsiege::damage_state const &actual, earthsiege::damage_state const &expected) {
  if(actual.internal != expected.internal) file.fail(label + " internal damage");
  if(actual.armour != expected.armour) file.fail(label + " armour damage");
  if(actual.present != expected.present) file.fail(label + " section presence");
  if(actual.cells != expected.cells) file.fail(label + " damage cells");
  if(actual.consequences != expected.consequences) file.fail(label + " consequences");
}

} // namespace

int main(int const argc, char **argv) try {
  if(argc != 3) return EXIT_FAILURE;
  earthsiege::resource_store resources{argv[1]};
  auto const weapons{earthsiege::read_weapon_catalogue(resources.read("simvol0", "dat/WEAPONS.DAT"))};
  auto const models{earthsiege::read_model(resources.read("simvol0", "dts/MECHWPNS.DTS"))};
  auto const explosions{earthsiege::read_explosions(resources.read("simvol0", "dat/EXPLOS.DAT"))};
  auto const debris{earthsiege::read_debris(resources.read("simvol0", "dat/DEBRIS.DAT"))};
  auto const effects{earthsiege::read_model(resources.read("simvol0", "dts/EXPLOS.DTS"))};
  auto const bytes{earthsiege::read_file(argv[2])}; earthsiege::binary_reader file{bytes};
  auto const magic{file.take(8)};
  if(std::string(magic.begin(), magic.end()) != "ESSTART1") file.fail("fixture version");
  auto const chassis{file.integer<uint16_t>()}; unsigned int starts{0}, restores{0};
  for(unsigned int entry{0}; entry != chassis; ++entry) {
    auto const name{file.cstring()}; auto const equipment{shorts(file)}; auto const values{shorts(file)};
    auto const definition{earthsiege::read_herc_definition(resources, name)};
    auto const count{file.integer<uint16_t>()};
    for(unsigned int trial{0}; trial != count; ++trial) {
      auto const label{name + '/' + std::to_string(trial)};
      auto const condition{file.integer<int16_t>()}; auto const initial{read_state(file)}; auto const expected{read_state(file)};
      std::vector<std::vector<int32_t>> expected_events(file.integer<uint16_t>());
      for(auto &event : expected_events) { event.resize(file.integer<uint8_t>()); for(auto &v : event) v = file.integer<int32_t>(); }
      earthsiege::herc_actor actor{definition, weapons, models, equipment, {0, 10000, 0}, {0, 0, 0}, values};
      compare_damage(file, label + " initial", actor.damage(), initial.damage);
      if(actor.allows_salvage() == initial.salvage_excluded) file.fail(label + " initial salvage");
      earthsiege::random_state random{initial.random}; std::vector<std::vector<int32_t>> events;
      earthsiege::herc_damage_events sink;
      sink.body_damage = [&](size_t const section, int16_t const amount){ events.push_back({0, amount, static_cast<int32_t>(section)}); };
      sink.explosion = [&](int16_t const id, vec3<int32_t> const &p, earthsiege::random_state &rng){
        events.push_back({1, id, p.x, p.y, p.z}); auto const &e{explosions.at(static_cast<size_t>(id))};
        earthsiege::create_explosion(e, std::get<earthsiege::shape_part>(effects.roots.at(static_cast<size_t>(e.model)).content).cell_frame_counts.size(), p, rng);
      };
      sink.debris = [&](int16_t const id, vec3<int32_t> const &p, earthsiege::random_state &rng){ events.push_back({2, id, p.x, p.y, p.z}); earthsiege::create_debris_burst(debris.at(static_cast<size_t>(id)), p, rng); };
      sink.attached_smoke = [&](int16_t const part, vec3<int16_t> const &p){ events.push_back({3, part, p.x, p.y, p.z}); };
      actor.apply_starting_condition(condition, random, sink);
      compare_damage(file, label, actor.damage(), expected.damage);
      if(events != expected_events) {
        for(auto const &e : events) { std::cerr << "actual:"; for(auto v : e) std::cerr << ' ' << v; std::cerr << '\n'; }
        for(auto const &e : expected_events) { std::cerr << "expected:"; for(auto v : e) std::cerr << ' ' << v; std::cerr << '\n'; }
        file.fail(label + " event order");
      }
      if(actor.allows_salvage() == expected.salvage_excluded || actor.body().movement().collapsed != expected.collapsed || actor.impacts().detectable != expected.detectable) file.fail(label + " actor flags");
      std::vector<int16_t> disabled;
      for(auto const &slot : actor.weapons().slots()) disabled.push_back(slot ? static_cast<int16_t>(slot->controls().disabled) : int16_t{-1});
      if(disabled != expected.weapons) file.fail(label + " weapon disabling");
      auto const &actual{random.snapshot()};
      if(actual.words != expected.random.words || actual.first != expected.random.first || actual.second != expected.random.second) file.fail(label + " shared randomness");
      ++starts;
    }
    auto const restore_count{file.integer<uint16_t>()};
    auto const loadout{earthsiege::assemble_herc_loadout(definition, equipment, weapons)};
    for(unsigned int trial{0}; trial != restore_count; ++trial) {
      earthsiege::herc_condition condition;
      for(auto &v : condition.armour) v = file.integer<int16_t>();
      for(auto &v : condition.internal) v = file.integer<int16_t>();
      for(auto &v : condition.equipment) v = file.integer<int16_t>();
      auto const initial{read_state(file)}; auto const expected{read_state(file)};
      earthsiege::component_damage damage{loadout.damage, initial.damage}; damage.restore_condition(condition);
      compare_damage(file, name + " restore/" + std::to_string(trial), damage.state(), expected.damage);
      ++restores;
    }
  }
  file.require_end();
  if(chassis != 16 || starts != 1792 || restores != 384) file.fail("starting-condition coverage differs");
  std::cout << starts << " original NPC conditions and " << restores << " player restores agree across all " << chassis << " HERC chassis" << std::endl;
  return EXIT_SUCCESS;
} catch(std::exception const &error) { std::cerr << error.what() << std::endl; return EXIT_FAILURE; }
