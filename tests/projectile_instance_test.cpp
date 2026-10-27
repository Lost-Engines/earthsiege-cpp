#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include "resources/resource_store.h"
#include "simulation/model_math.h"
#include "simulation/projectile_instance.h"

namespace {

template<typename T> vec3<T> vector(earthsiege::binary_reader &file) { return {file.integer<T>(), file.integer<T>(), file.integer<T>()}; }

void check_packet(earthsiege::binary_reader &file, earthsiege::impact_request const &request, std::string const &label) {
  auto const require{[&](bool const correct, char const *field){ if(!correct) file.fail(label + ": " + field); }};
  for(unsigned int const i : {0U, 1U, 3U, 4U, 2U, 5U, 6U, 7U, 8U}) require(request.transform.basis.data[i] == file.integer<int16_t>(), "basis");
  require(request.transform.type == file.integer<uint8_t>(), "matrix type");
  file.integer<uint8_t>();
  require(request.transform.translation == vector<int32_t>(file), "translation");
  require(request.reach == file.integer<int32_t>(), "reach");
  require(request.radius == file.integer<int16_t>(), "radius");
  require(request.damage.armour == file.integer<int16_t>(), "armour damage");
  require(request.damage.shield == file.integer<int16_t>(), "shield damage");
  require(request.damage.splash_q10 == file.integer<int16_t>(), "splash");
  for(auto const &table : {request.damage.shield_effects, request.damage.ground_effects, request.damage.armour_effects}) for(auto const effect : table) require(effect == file.integer<int16_t>(), "impact effect");
  require(request.damage.missile_subtype.value_or(5) == file.integer<int16_t>(), "missile signature");
}

} // namespace

int main(int const argc, char **argv) try {
  if(argc != 3) return EXIT_FAILURE;
  earthsiege::resource_store resources{argv[1]};
  auto const definitions{earthsiege::read_weapon_catalogue(resources.read("simvol0", "dat/WEAPONS.DAT"))};
  earthsiege::projectile_motions const motions{earthsiege::read_bullet_motions(resources.read("simvol0", "dat/BULLETS.DAT")), earthsiege::read_rocket_motions(resources.read("simvol0", "dat/ROCKETS.DAT"))};
  auto const bullets{earthsiege::read_model(resources.read("simvol0", "dts/BULLETS.DTS"))};
  auto const rockets{earthsiege::read_model(resources.read("simvol0", "dts/ROCKETS.DTS"))};
  auto const bytes{earthsiege::read_file(argv[2])};
  earthsiege::binary_reader file{bytes};
  auto const magic{file.take(8)};
  if(std::string(magic.begin(), magic.end()) != "ESPKTS01") file.fail("fixture version");
  auto const flight_count{file.integer<uint32_t>()};
  auto const beam_count{file.integer<uint32_t>()};
  auto const lookup{[&](earthsiege::projectile_kind const kind, int16_t const subtype){
    for(size_t i{0}; i != definitions.projectiles.size(); ++i) if(definitions.projectiles[i].kind == kind && definitions.projectiles[i].subtype == subtype) return i;
    throw std::runtime_error{"no projectile definition"};
  }};
  for(unsigned int row{0}; row != flight_count; ++row) {
    auto const kind{static_cast<earthsiege::projectile_kind>(file.integer<int16_t>())};
    auto const subtype{file.integer<int16_t>()};
    auto const charge{file.integer<int16_t>()};
    auto const ratio{file.integer<int16_t>()};
    bool const hit{file.integer<uint8_t>() != 0};
    auto const angles{vector<uint16_t>(file)};
    auto const position{vector<int32_t>(file)};
    bool const done{file.integer<uint8_t>() != 0};
    auto const final_position{vector<int32_t>(file)};
    earthsiege::weapon_shot shot;
    shot.kind = kind == earthsiege::projectile_kind::rocket ? earthsiege::shot_kind::rocket : earthsiege::shot_kind::scaled;
    shot.projectile = lookup(kind, subtype); shot.charge = charge;
    shot.muzzle.position = position; shot.muzzle.angles = angles;
    earthsiege::projectile_instance projectile{shot, definitions, motions, bullets, rockets};
    earthsiege::projectile_guidance guidance;
    unsigned int queries{0};
    auto const outcome{projectile.advance(ratio, guidance, [&](earthsiege::impact_request const &request) -> std::optional<int32_t> {
      ++queries; check_packet(file, request, "flight " + std::to_string(row));
      return hit ? std::optional<int32_t>{100} : std::nullopt;
    })};
    if(queries != 1 || (outcome != earthsiege::projectile_result::flying) != done || projectile.flight().position != final_position) file.fail("flight result differs");
    if(!std::ranges::equal(projectile.model().cell_frames(), projectile.flight().cells)) file.fail("projectile visual cells differ");
  }
  for(unsigned int row{0}; row != beam_count; ++row) {
    auto const subtype{file.integer<int16_t>()};
    auto const charge{file.integer<int16_t>()};
    auto const reach{file.integer<int32_t>()};
    auto const depth{file.integer<int32_t>()};
    auto const angles{vector<uint16_t>(file)};
    auto const position{vector<int32_t>(file)};
    auto const start{vector<int32_t>(file)};
    auto const end{vector<int32_t>(file)};
    earthsiege::weapon_shot shot;
    shot.kind = earthsiege::shot_kind::beam; shot.projectile = lookup(earthsiege::projectile_kind::beam, subtype); shot.charge = charge; shot.range = reach;
    shot.muzzle.transform = earthsiege::rotation_transform(angles); shot.muzzle.transform.translation = position; shot.muzzle.position = position; shot.muzzle.angles = angles;
    unsigned int queries{0};
    auto const path{earthsiege::dispatch_beam(shot, definitions, [&](earthsiege::impact_request const &request) -> std::optional<int32_t> {
      ++queries; check_packet(file, request, "beam " + std::to_string(row));
      return depth ? std::optional<int32_t>{depth - 1} : std::nullopt;
    })};
    if(queries != 1 || path.start != start || path.end != end) file.fail("beam endpoint differs");
  }
  file.require_end();
  if(flight_count != 270 || beam_count != 576) file.fail("packet reference coverage differs");
  std::cout << flight_count << " projectile updates and " << beam_count << " instant beams match original hit packets" << std::endl;
  return EXIT_SUCCESS;
} catch(std::exception const &error) {
  std::cerr << error.what() << std::endl;
  return EXIT_FAILURE;
}
