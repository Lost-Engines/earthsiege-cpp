#include "resources/resource_store.h"
#include "simulation/beam_effect.h"
#include "simulation/weapon_display.h"
#include "simulation/weapon_firing.h"
#include <cstdlib>
#include <iostream>
#include <stdexcept>
using namespace earthsiege;
namespace {
void require(bool pass, std::string const &label) {
  if (!pass)
    throw std::runtime_error{label};
}
template <class T> void eq(binary_reader &r, T actual, std::string const &label) {
  require(actual == r.integer<T>(), label);
}
vec3<int32_t> point(binary_reader &r) { return {r.integer<int32_t>(), r.integer<int32_t>(), r.integer<int32_t>()}; }
random_state::state rng_state(binary_reader &r) {
  random_state::state s;
  for (auto &v : s.words)
    v = r.integer<uint16_t>();
  s.first = r.integer<uint8_t>();
  s.second = r.integer<uint8_t>();
  return s;
}
} // namespace
int main(int argc, char **argv) try {
  if (argc != 4)
    return EXIT_FAILURE;
  resource_store resources{argv[1]};
  auto definitions{read_weapon_catalogue(resources)};
  auto const models{read_model(resources.read("simvol0", "dts/MECHWPNS.DTS"))};
  weapon_mount mount{.orientation{4}, .equipment_slot{0}};
  for (int16_t id{}; id < 22; ++id) {
    std::array equipment{id}, values{int16_t{5}};
    std::array mounts{mount};
    weapon_bank bank{definitions, models, mounts, equipment, values};
    if (id == 17) {
      require(!bank.slots().front(), "empty slot remains 17");
      continue;
    }
    auto const &w{bank.at(0)};
    // Enemy-only MISSL has no cockpit seeker label in the original table.
    if(id != 16) require(!weapon_display_name(w, definitions).empty(), "weapon instrument name");
    if (id == 9 || id == 10 || id == 11)
      require(w.projectile && definitions.projectiles[*w.projectile].kind == projectile_kind::rocket,
              "missile selector 34");
    else if (id == 5 || id == 13)
      require(!w.projectile, "non-projectile selectors");
    else
      require(w.projectile == static_cast<size_t>(definitions.weapons[id].projectile), "projectile selector");
  }
  auto weapon_fixture{read_file(argv[2])};
  binary_reader f{weapon_fixture};
  auto m{f.take(8)};
  require(std::string(m.begin(), m.end()) == "ESEXWPN1", "weapon fixture");
  auto count{f.integer<uint16_t>()};
  for (unsigned i{}; i < count; ++i) {
    auto id{f.integer<int16_t>()};
    auto steps{f.integer<uint16_t>()};
    capacitor c;
    c.charge = 961;
    weapon_state state{
        .definition{id}, .projectile{static_cast<size_t>(definitions.weapons[id].projectile)}, .mechanism{c}, .convergence{}};
    for (unsigned j{}; j < steps; ++j) {
      auto shots{discharge_weapon(state, definitions, mount, {}, false)};
      eq(f, state.energy()->charge, "original EMP charge");
      eq(f, static_cast<uint8_t>(state.pending_second_shot), "original EMP second-shot latch");
      eq(f, state.controls().refire_remaining, "original EMP refire");
      require(shots.size() == 1, "EMP spawn count");
      eq(f, shots[0].charge, "original EMP projectile charge");
      eq(f, definitions.projectiles[shots[0].projectile].subtype, "original EMP subtype");
    }
  }
  auto lifetimes{f.integer<uint16_t>()};
  for (unsigned i{}; i < lifetimes; ++i) {
    auto expansion{f.integer<uint8_t>()};
    auto ratio{f.integer<int16_t>()};
    auto steps{f.integer<uint16_t>()};
    projectile_motions motions;
    motions.edition = expansion ? game_edition::expansion : game_edition::original;
    motions.bullets.push_back({.lifetime{4}});
    projectile_flight flight;
    flight.position = {0, 0, 10000};
    projectile_guidance guidance;
    for (unsigned j{}; j < steps; ++j) {
      auto result{advance_projectile(flight, motions, 1000, {}, ratio, guidance)};
      eq(f, flight.age, "original bullet age");
      eq(f, static_cast<uint8_t>(result == projectile_result::expired), "original bullet expiry");
    }
  }
  f.require_end();
  auto beam_fixture{read_file(argv[3])};
  binary_reader b{beam_fixture};
  auto bm{b.take(8)};
  require(std::string(bm.begin(), bm.end()) == "ESEXBEAM", "beam fixture");
  auto styles{read_beam_styles(resources.read("simvol0", "dat/BEAM.DAT"))};
  auto beams{b.integer<uint16_t>()};
  for (unsigned i{}; i < beams; ++i) {
    auto type{b.integer<int16_t>()};
    beam_path path{point(b), point(b)};
    auto colour{b.integer<int16_t>()};
    random_state rng{rng_state(b)};
    auto expected{rng_state(b)};
    auto beam{make_beam_effect(type, path, rng, styles, game_edition::expansion)};
    require(beam.colour == colour, "beam colour");
    require(rng.snapshot().words == expected.words && rng.snapshot().first == expected.first &&
                rng.snapshot().second == expected.second,
            "beam random draws");
    eq(b, static_cast<uint16_t>(beam.points.size()), "beam vertices");
    for (auto const &v : beam.points)
      require(v == point(b), "original beam vertex " + std::to_string(i));
    eq(b, static_cast<uint16_t>(beam.faces.size()), "beam faces");
    auto strip{b.integer<uint16_t>()};
    require(strip == (type == 1 || type == 7), "ELF-II strip");
    eq(b, beam.remaining, "original beam lifetime");
    auto steps{b.integer<uint16_t>()};
    for (unsigned j{}; j < steps; ++j) {
      auto ratio{b.integer<int16_t>()};
      auto done{advance_beam_effect(beam, ratio)};
      eq(b, beam.remaining, "beam remaining");
      eq(b, static_cast<uint8_t>(done), "beam expiry");
    }
  }
  b.require_end();
  std::cout << "All expansion weapon selectors, " << count << " EMP sequences, " << lifetimes
            << " bullet lifetime traces and " << beams << " original beam geometry/random/lifetime traces agree\n";
  return EXIT_SUCCESS;
} catch (std::exception const &e) {
  std::cerr << e.what() << '\n';
  return EXIT_FAILURE;
}
