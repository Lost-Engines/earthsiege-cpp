#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include "resources/resource_store.h"
#include "simulation/model_math.h"
#include "simulation/world_projectiles.h"

namespace {

template<typename T> vec3<T> vector(earthsiege::binary_reader &file) { return {file.integer<T>(), file.integer<T>(), file.integer<T>()}; }
std::vector<uint16_t> words(earthsiege::binary_reader &file) {
  std::vector<uint16_t> result(file.integer<uint16_t>());
  for(auto &v : result) v = file.integer<uint16_t>();
  return result;
}
earthsiege::random_state::state random(earthsiege::binary_reader &file) {
  earthsiege::random_state::state result;
  for(auto &word : result.words) word = file.integer<uint16_t>();
  result.first = file.integer<uint8_t>(); result.second = file.integer<uint8_t>();
  return result;
}
std::vector<uint16_t> order(earthsiege::object_pool<earthsiege::live_projectile> const &pool, bool const retiring) {
  std::vector<uint16_t> result;
  pool.for_each([&](size_t const slot, earthsiege::live_projectile const &){ if(!retiring || pool.retiring(slot)) result.push_back(static_cast<uint16_t>(slot)); });
  std::ranges::reverse(result);
  return result;
}

} // anonymous namespace

int main(int const argc, char **argv) try {
  if(argc != 3) return EXIT_FAILURE;
  earthsiege::resource_store resources{argv[1]};
  auto const definitions{earthsiege::read_weapon_catalogue(resources.read("simvol0", "dat/WEAPONS.DAT"))};
  auto const bytes{earthsiege::read_file(argv[2])}; earthsiege::binary_reader file{bytes};
  auto const magic{file.take(8)};
  if(std::string(magic.begin(), magic.end()) != "ESPRWR01") file.fail("fixture version");
  auto const traces{file.integer<uint16_t>()}; size_t frames{0};
  for(size_t trace{0}; trace != traces; ++trace) {
    auto const ratio{file.integer<int16_t>()};
    earthsiege::random_state rng{random(file)};
    earthsiege::world_projectiles world{resources, definitions};
    auto const count{file.integer<uint16_t>()};
    for(size_t row{0}; row != count; ++row, ++frames) {
      auto const require{[&](bool const value, char const *field){ if(!value) file.fail("projectile " + std::to_string(trace) + '/' + std::to_string(row) + ": " + field); }};
      auto const depth{file.integer<int32_t>()}; auto const expected_hits{file.integer<uint16_t>()};
      auto const before{words(file)}, retired{words(file)};
      require(order(world.live(), false) == before, "update order");
      size_t hits{0};
      earthsiege::world_impact_query const hit{[&](earthsiege::projectile_owner const &owner, earthsiege::impact_request const &) -> std::optional<int32_t> {
        require(owner.actor == 0 && !owner.target, "owner"); ++hits;
        return depth ? std::optional<int32_t>{depth - 1} : std::nullopt;
      }};
      world.advance(ratio, [](earthsiege::projectile_owner &){ earthsiege::projectile_guidance value; value.player_owner = true; return value; }, hit);
      require(order(world.live(), true) == retired, "retirement order");
      auto const requests{file.integer<uint16_t>()};
      for(size_t request{0}; request != requests; ++request) {
        earthsiege::weapon_shot shot;
        shot.kind = static_cast<earthsiege::shot_kind>(file.integer<uint8_t>());
        shot.projectile = file.integer<uint16_t>(); shot.charge = file.integer<int16_t>(); shot.range = file.integer<int32_t>();
        shot.muzzle.position = vector<int32_t>(file); shot.muzzle.angles = vector<uint16_t>(file);
        shot.muzzle.transform = earthsiege::rotation_transform(shot.muzzle.angles); shot.muzzle.transform.translation = shot.muzzle.position;
        auto const slot{file.integer<uint16_t>()};
        require(world.launch({0, std::nullopt}, shot, rng, hit) == slot, "launch slot");
      }
      world.collect();
      require(hits == expected_hits, "hit call count");
      require(world.live().size() == file.integer<uint16_t>(), "live count");
      world.live().for_each([&](size_t const slot, earthsiege::live_projectile const &entry){
        require(slot == file.integer<uint16_t>(), "live slot order");
        bool const is_beam{file.integer<uint8_t>() != 0};
        require(std::holds_alternative<earthsiege::beam_effect>(entry.state) == is_beam, "kind");
        if(is_beam) require(std::get<earthsiege::beam_effect>(entry.state).points.size() == file.integer<uint16_t>(), "beam vertices");
        else {
          auto const &value{std::get<earthsiege::projectile_instance>(entry.state).flight()};
          require(static_cast<int16_t>(value.kind) == file.integer<int16_t>() && value.subtype == file.integer<uint8_t>(), "flight subtype");
          require(value.position == vector<int32_t>(file) && value.angles == vector<uint16_t>(file), "flight transform");
          require(value.speed == file.integer<int16_t>() && value.age == file.integer<int16_t>() && value.animation_remaining == file.integer<int16_t>(), "flight speed/clock");
          require(value.cells == words(file), "flight cells");
        }
      });
      auto const expected{random(file)}; auto const &actual{rng.snapshot()};
      require(actual.words == expected.words && actual.first == expected.first && actual.second == expected.second, "shared random state");
    }
  }
  file.require_end();
  std::cout << frames << " original mixed projectile frames agree, including beams, impacts, slot reuse and shared randomness\n";
  return EXIT_SUCCESS;
} catch(std::exception const &error) {
  std::cerr << error.what() << '\n';
  return EXIT_FAILURE;
}
