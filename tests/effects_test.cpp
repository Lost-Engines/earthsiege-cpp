#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include "resources/resource_store.h"
#include "resources/model.h"
#include "simulation/effect_state.h"

namespace {

template<typename T> vec3<T> vector(earthsiege::binary_reader &file) { return {file.integer<T>(), file.integer<T>(), file.integer<T>()}; }
std::vector<int16_t> shorts(earthsiege::binary_reader &file) {
  std::vector<int16_t> result(file.integer<uint16_t>());
  for(auto &v : result) v = file.integer<int16_t>();
  return result;
}
earthsiege::random_state::state random_state(earthsiege::binary_reader &file) {
  earthsiege::random_state::state result;
  for(auto &v : result.words) v = file.integer<uint16_t>();
  result.first = file.integer<uint8_t>(); result.second = file.integer<uint8_t>();
  return result;
}
void check_random(earthsiege::binary_reader &file, earthsiege::random_state const &random) {
  auto const expected{random_state(file)}; auto const &actual{random.snapshot()};
  if(actual.words != expected.words || actual.first != expected.first || actual.second != expected.second) file.fail("effect random state differs");
}
void check_state(earthsiege::binary_reader &file, earthsiege::explosion_state const &state) {
  auto const timer{file.integer<int16_t>()}; auto const cells{shorts(file)};
  auto const secondary{file.integer<int16_t>()}; auto const light{file.integer<int16_t>()};
  if(state.remaining != timer || !std::ranges::equal(state.cells, cells) || (state.secondary_cell ? static_cast<int>(*state.secondary_cell) : -1) != secondary || state.light_intensity.value_or(-1) != light) file.fail("explosion animation state differs");
}

} // namespace

int main(int const argc, char **argv) try {
  if(argc != 3) return EXIT_FAILURE;
  earthsiege::resource_store resources{argv[1]};
  auto const explosions{earthsiege::read_explosions(resources.read("simvol0", "dat/EXPLOS.DAT"))};
  auto const debris{earthsiege::read_debris(resources.read("simvol0", "dat/DEBRIS.DAT"))};
  auto const models{earthsiege::read_model(resources.read("simvol0", "dts/EXPLOS.DTS"))};
  auto const bytes{earthsiege::read_file(argv[2])}; earthsiege::binary_reader file{bytes};
  auto const magic{file.take(8)};
  if(std::string(magic.begin(), magic.end()) != "ESFX0001") file.fail("fixture version");
  auto const creation_count{file.integer<uint32_t>()}, burst_count{file.integer<uint32_t>()}, trace_count{file.integer<uint32_t>()};
  auto const position{vector<int32_t>(file)};
  for(unsigned int row{0}; row != creation_count; ++row) {
    auto const &definition{explosions.at(file.integer<uint16_t>())};
    auto const &shape{std::get<earthsiege::shape_part>(models.roots.at(static_cast<size_t>(definition.model)).content)};
    earthsiege::random_state random{random_state(file)};
    auto const actual{earthsiege::create_explosion(definition, shape.cell_frame_counts.size(), position, random)};
    auto const kind{file.integer<int16_t>()}; auto const angles{vector<uint16_t>(file)}; auto const expected_position{vector<int32_t>(file)};
    auto const timer{file.integer<int16_t>()}; auto const cells{shorts(file)};
    bool const secondary{file.integer<uint8_t>() != 0}; auto const light{file.integer<int16_t>()}; auto const sound{file.integer<int16_t>()};
    if(kind != (definition.alternate_object_kind ? 8 : 2) || actual.angles != angles || actual.position != expected_position || actual.remaining != timer || !std::ranges::equal(actual.cells, cells) || actual.secondary_cell.has_value() != secondary || actual.light_intensity.value_or(-1) != light || actual.sound != sound) file.fail("explosion constructor " + std::to_string(row) + " differs");
    check_random(file, random);
  }
  unsigned int particles{0};
  for(unsigned int row{0}; row != burst_count; ++row) {
    auto const &definition{debris.at(file.integer<uint16_t>())}; earthsiege::random_state random{random_state(file)};
    auto const actual{earthsiege::create_debris_burst(definition, position, random)};
    auto const count{file.integer<uint16_t>()}; if(actual.size() != count) file.fail("debris burst size");
    for(auto const &particle : actual) {
      auto const model{file.integer<int16_t>()}; auto const expected_position{vector<int32_t>(file)};
      auto const angles{vector<uint16_t>(file)}; auto const velocity{vector<int16_t>(file)};
      if(particle.model != model || particle.position != expected_position || particle.angles != angles || particle.velocity != velocity) file.fail("debris launch differs");
      ++particles;
    }
    check_random(file, random);
  }
  unsigned int updates{0};
  for(unsigned int row{0}; row != trace_count; ++row) {
    auto const &definition{explosions.at(file.integer<uint16_t>())}; auto const ratio{file.integer<int16_t>()}; auto const secondary_count{file.integer<uint16_t>()};
    auto const counts{shorts(file)};
    auto const &shape{std::get<earthsiege::shape_part>(models.roots.at(static_cast<size_t>(definition.model)).content)};
    if(counts != shape.cell_frame_counts) file.fail("effect frame counts differ");
    earthsiege::random_state random;
    auto state{earthsiege::create_explosion(definition, counts.size(), position, random)};
    check_state(file, state);
    auto const count{file.integer<uint16_t>()};
    for(unsigned int step{0}; step != count; ++step) {
      bool const done{earthsiege::advance_explosion(state, definition, counts, secondary_count, ratio)};
      check_state(file, state);
      if(done != (file.integer<uint8_t>() != 0)) file.fail("explosion expiry differs");
      ++updates;
    }
  }
  file.require_end();
  if(explosions.size() != 18 || debris.size() != 4 || creation_count != 1152 || burst_count != 256 || trace_count != 126 || updates != 3046) file.fail("effect reference coverage differs");
  std::cout << creation_count << " original explosions, " << burst_count << " debris bursts (" << particles << " particles), and " << updates << " complete animation/light updates agree" << std::endl;
  return EXIT_SUCCESS;
} catch(std::exception const &error) {
  std::cerr << error.what() << std::endl;
  return EXIT_FAILURE;
}
