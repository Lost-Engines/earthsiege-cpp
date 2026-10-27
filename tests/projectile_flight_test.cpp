#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include "resources/resource_store.h"
#include "simulation/projectile_flight.h"

namespace {

std::vector<int16_t> shorts(earthsiege::binary_reader &file) {
  std::vector<int16_t> values(file.integer<uint16_t>());
  for(auto &value : values) value = file.integer<int16_t>();
  return values;
}

earthsiege::projectile_flight read_state(earthsiege::binary_reader &file) {
  earthsiege::projectile_flight state;
  state.position = {file.integer<int32_t>(), file.integer<int32_t>(), file.integer<int32_t>()};
  state.angles = {file.integer<uint16_t>(), file.integer<uint16_t>(), file.integer<uint16_t>()};
  state.speed = file.integer<int16_t>(); state.age = file.integer<int16_t>(); state.animation_remaining = file.integer<int16_t>();
  auto const cells{shorts(file)};
  for(auto const cell : cells) state.cells.push_back(static_cast<uint16_t>(cell));
  return state;
}

} // namespace

int main(int const argc, char **argv) try {
  if(argc != 3) return EXIT_FAILURE;
  earthsiege::resource_store resources{argv[1]};
  earthsiege::projectile_motions const motions{
    earthsiege::read_bullet_motions(resources.read("simvol0", "dat/BULLETS.DAT")),
    earthsiege::read_rocket_motions(resources.read("simvol0", "dat/ROCKETS.DAT")),
  };
  auto const bytes{earthsiege::read_file(argv[2])};
  earthsiege::binary_reader file{bytes};
  auto const magic{file.take(8)};
  if(std::string(magic.begin(), magic.end()) != "ESFLY001") file.fail("fixture version");
  auto const traces{file.integer<uint16_t>()};
  unsigned int total{0};
  for(unsigned int trace{0}; trace != traces; ++trace) {
    auto const kind{static_cast<earthsiege::projectile_kind>(file.integer<int16_t>())};
    auto const subtype{file.integer<uint8_t>()};
    auto const ratio{file.integer<int16_t>()};
    auto const speed_limit{file.integer<int16_t>()};
    auto const mode{file.cstring()};
    auto const spec{shorts(file)};
    std::vector<int16_t> decoded;
    if(kind == earthsiege::projectile_kind::bullet) {
      auto const &m{motions.bullets.at(subtype)};
      decoded = {m.model, m.lifetime, m.collision_radius, m.animation_period, m.shot_noise};
    } else {
      auto const &m{motions.rockets.at(subtype)};
      decoded = {m.model, m.lifetime, m.acceleration, m.collision_radius, m.animation_period, m.cell_sequence, m.shot_noise};
    }
    if(decoded != spec) file.fail("projectile motion resource differs");
    auto const counts{shorts(file)};
    std::vector<uint16_t> cell_counts;
    for(auto const n : counts) cell_counts.push_back(static_cast<uint16_t>(n));
    auto state{read_state(file)};
    state.kind = kind; state.subtype = subtype;
    state.target_present = kind == earthsiege::projectile_kind::rocket && mode != "none";
    earthsiege::projectile_guidance input;
    auto const steps{file.integer<uint16_t>()};
    for(unsigned int step{0}; step != steps; ++step) {
      input.target_position = {file.integer<int32_t>(), file.integer<int32_t>(), file.integer<int32_t>()};
      input.player_owner = file.integer<uint8_t>() != 0;
      input.owner_tracks_target = file.integer<uint8_t>() != 0;
      input.target_detectable = file.integer<uint8_t>() != 0;
      input.interference = file.integer<uint8_t>() != 0;
      input.manual_held = file.integer<uint8_t>() != 0;
      input.manual_axes = {file.integer<int16_t>(), file.integer<int16_t>()};
      auto const expected{read_state(file)};
      bool const done{file.integer<uint8_t>() != 0};
      auto const expected_type{file.integer<uint8_t>()};
      bool const target{file.integer<uint8_t>() != 0};
      bool const owner_guiding{file.integer<uint8_t>() != 0};
      bool const held{file.integer<uint8_t>() != 0};
      vec2<int16_t> const axes{file.integer<int16_t>(), file.integer<int16_t>()};
      auto const result{earthsiege::advance_projectile(state, motions, speed_limit, cell_counts, ratio, input)};
      if(state.position != expected.position || state.angles != expected.angles || state.speed != expected.speed || state.age != expected.age || state.animation_remaining != expected.animation_remaining || state.cells != expected.cells || state.subtype != expected_type || state.target_present != target || input.owner_guiding != owner_guiding || input.manual_held != held || input.manual_axes != axes || (result != earthsiege::projectile_result::flying) != done) {
        file.fail("projectile trace " + std::to_string(trace) + " mode " + mode + " step " + std::to_string(step) + " differs");
      }
      ++total;
    }
  }
  file.require_end();
  if(traces != 216 || total != 14895) file.fail("projectile reference coverage differs");
  std::cout << total << " original projectile flight updates agree across " << traces << " trajectories" << std::endl;
  return EXIT_SUCCESS;
} catch(std::exception const &error) {
  std::cerr << error.what() << std::endl;
  return EXIT_FAILURE;
}
