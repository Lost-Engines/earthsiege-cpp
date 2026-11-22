#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include "resources/career.h"
#include "resources/name_table.h"
#include "resources/resource_store.h"
#include "simulation/herc_actor.h"
#include "simulation/mission_salvage.h"
using earthsiege::assess_salvage;
using earthsiege::binary_reader;
using earthsiege::damage_state;
using earthsiege::herc_actor;
using earthsiege::normalise_path;
using earthsiege::read_file;
using earthsiege::read_hangar;
using earthsiege::read_herc_definition;
using earthsiege::read_model;
using earthsiege::read_resource_names;
using earthsiege::read_weapon_catalogue;
using earthsiege::resource_store;
using earthsiege::returned_condition;
using earthsiege::salvage_weapon;
void require(bool ok, std::string const &s) {
  if(!ok) throw std::runtime_error{s};
}
std::vector<int16_t> words(binary_reader &f) {
  std::vector<int16_t> out(f.integer<uint16_t>());
  for(auto &v : out) v = f.integer<int16_t>();
  return out;
}
int main(int argc, char **argv) try {
  if(argc != 3) return EXIT_FAILURE;
  resource_store resources{argv[1]};
  auto const roster{read_hangar(resources.read("shell0", "car/sm_init.dat"))};
  auto const weapons{read_weapon_catalogue(resources.read("simvol0", "dat/weapons.dat"))};
  auto const templates{read_model(resources.read("simvol0", "dts/mechwpns.dts"))};
  auto const names{read_resource_names(resources.read("simvol0", "nam/mechs.nam"))};
  auto const bytes{read_file(argv[2])};
  binary_reader f{bytes};
  auto const magic{f.take(8)};
  require(std::string(magic.begin(), magic.end()) == "ESRES001", "fixture version");
  auto const models{f.integer<uint16_t>()};
  size_t total{0};
  for(unsigned i{0}; i < models; ++i) {
    auto const name{f.cstring()};
    auto const value{f.integer<int16_t>()};
    auto const cases{f.integer<uint32_t>()};
    auto const id{std::ranges::find_if(names, [&](auto const &n){return normalise_path(n) == normalise_path(name);}) - names.begin()};
    auto const h{roster.find(static_cast<int16_t>(id))};
    require(h != nullptr, "fixture chassis");
    auto const body{read_herc_definition(resources, name)};
    herc_actor actor{body, weapons, templates, h->weapons, {}, {}};
    std::vector<salvage_weapon> equipped;
    for(auto const &w : actor.weapons().slots()) {
      if(w) equipped.push_back({w->equipment_slot, w->definition});
    }
    for(unsigned c{0}; c < cases; ++c) {
      damage_state s;
      s.internal = words(f);
      s.armour = words(f);
      s.present = words(f);
      std::array<int16_t, 36> expected;
      for(auto &v : expected) v = f.integer<int16_t>();
      auto const score{f.integer<int32_t>()};
      auto const count{f.integer<uint16_t>()};
      std::vector<std::pair<int16_t, int16_t>> salvage;
      for(unsigned k{0}; k < count; ++k) salvage.emplace_back(f.integer<int16_t>(), int16_t{}), salvage.back().second = f.integer<int16_t>();
      auto const actual{returned_condition(actor.definition().damage, s)};
      auto label{name + "/" + std::to_string(c)};
      require(std::equal(actual.armour.begin(), actual.armour.end(), expected.begin()), label + " armor");
      require(std::equal(actual.internal.begin(), actual.internal.end(), expected.begin() + 13), label + " internal");
      require(std::equal(actual.equipment.begin(), actual.equipment.begin() + 9, expected.begin() + 23), label + " equipment");
      require(actual.equipment[9] == 100, "unallocated final equipment slot");
      auto const recovered{assess_salvage(actor.definition().damage, s, equipped, value)};
      require(recovered.score == score && recovered.weapons == salvage, label + " salvage");
      auto const excluded{assess_salvage(actor.definition().damage, s, equipped, value, true)};
      require(!excluded.score && excluded.weapons.empty(), "excluded actor salvage");
      ++total;
    }
  }
  f.require_end();
  std::cout << total << " native return conditions and recovery/salvage records match; undefined tenth equipment word replaced safely\n";
  return EXIT_SUCCESS;
} catch(std::exception const &e) {
  std::cerr << e.what() << '\n';
  return EXIT_FAILURE;
}
