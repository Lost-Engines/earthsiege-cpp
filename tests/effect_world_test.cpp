#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include "resources/resource_store.h"
#include "simulation/world_effects.h"

namespace {

template<typename T> vec3<T> vector(earthsiege::binary_reader &file) { return {file.integer<T>(), file.integer<T>(), file.integer<T>()}; }
std::vector<uint16_t> words(earthsiege::binary_reader &file) {
  std::vector<uint16_t> result(file.integer<uint16_t>());
  for(auto &v : result) v = file.integer<uint16_t>();
  return result;
}
earthsiege::random_state::state random_state(earthsiege::binary_reader &file) {
  earthsiege::random_state::state result;
  for(auto &v : result.words) v = file.integer<uint16_t>();
  result.first = file.integer<uint8_t>(); result.second = file.integer<uint8_t>();
  return result;
}
template<typename T> std::vector<uint16_t> order(earthsiege::object_pool<T> const &pool, bool const retiring) {
  std::vector<uint16_t> result;
  pool.for_each([&](size_t const slot, T const &){ if(!retiring || pool.retiring(slot)) result.push_back(static_cast<uint16_t>(slot)); });
  std::ranges::reverse(result);                                                // fixture contains the original 0x63a20 oldest-first walk
  return result;
}

} // namespace

int main(int const argc, char **argv) try {
  if(argc != 3) return EXIT_FAILURE;
  earthsiege::resource_store resources{argv[1]};
  auto const bytes{earthsiege::read_file(argv[2])}; earthsiege::binary_reader file{bytes};
  auto const magic{file.take(8)};
  if(std::string(magic.begin(), magic.end()) != "ESFXWR01") file.fail("fixture version");
  auto const traces{file.integer<uint16_t>()}; unsigned int frames{0}, observed{0};
  for(unsigned int trace{0}; trace != traces; ++trace) {
    auto const ratio{file.integer<int16_t>()}; earthsiege::random_state random{random_state(file)};
    earthsiege::world_effects world{resources}; auto const frame_count{file.integer<uint16_t>()};
    for(unsigned int frame{0}; frame != frame_count; ++frame) {
      auto const label{std::to_string(trace) + '/' + std::to_string(frame)};
      auto const requests{file.integer<uint16_t>()};
      for(unsigned int r{0}; r != requests; ++r) {
        auto const kind{file.integer<int32_t>()}, definition{file.integer<int32_t>()}; auto const position{vector<int32_t>(file)};
        if(kind == 0) {
          if(!world.add_explosion(static_cast<int16_t>(definition), position, random)) file.fail(label + " unexpected full explosion pool");
        } else world.add_debris(static_cast<int16_t>(definition), position, random);
      }
      auto const explosion_order{words(file)}, debris_order{words(file)}, retired_explosions{words(file)}, retired_debris{words(file)};
      if(order(world.live_explosions(), false) != explosion_order || order(world.live_particles(), false) != debris_order) file.fail(label + " effect update order");
      world.advance(ratio);
      if(order(world.live_explosions(), true) != retired_explosions || order(world.live_particles(), true) != retired_debris) file.fail(label + " effect retirement order");
      world.collect();
      auto const explosions{file.integer<uint16_t>()};
      if(world.live_explosions().size() != explosions) file.fail(label + " explosion count");
      world.live_explosions().for_each([&](size_t const slot, earthsiege::live_explosion const &effect){
        auto const expected_slot{file.integer<uint16_t>()}; auto const definition{file.integer<int16_t>()};
        auto const position{vector<int32_t>(file)}; auto const angles{vector<uint16_t>(file)}; auto const timer{file.integer<int16_t>()};
        auto const cells{words(file)}; auto const secondary{file.integer<int16_t>()}, light{file.integer<int16_t>()};
        auto const &state{effect.state};
        if(slot != expected_slot || effect.definition != definition || state.position != position || state.angles != angles || state.remaining != timer || state.cells != cells || state.secondary_cell.value_or(65535) != static_cast<uint16_t>(secondary) || state.light_intensity.value_or(-1) != light) { std::cerr << "slot " << slot << "/" << expected_slot << " def " << effect.definition << "/" << definition << " timer " << state.remaining << "/" << timer << " secondary " << state.secondary_cell.value_or(65535) << "/" << secondary << " light " << state.light_intensity.value_or(-1) << "/" << light << "\n"; file.fail(label + " explosion state"); }
        if(!std::ranges::equal(effect.model.cell_frames(), cells) || (effect.secondary ? effect.secondary->cell_frames().front() : 65535) != static_cast<uint16_t>(secondary)) file.fail(label + " owned effect geometry cells");
        ++observed;
      });
      auto const debris{file.integer<uint16_t>()};
      if(world.live_particles().size() != debris) file.fail(label + " debris count");
      world.live_particles().for_each([&](size_t const slot, earthsiege::live_debris const &effect){
        auto const expected_slot{file.integer<uint16_t>()}; auto const model{file.integer<int16_t>()};
        auto const position{vector<int32_t>(file)}; auto const angles{vector<uint16_t>(file)}; auto const velocity{vector<int16_t>(file)};
        auto const &state{effect.state};
        if(slot != expected_slot || state.model != model || state.position != position || state.angles != angles || state.velocity != velocity) file.fail(label + " debris state");
        ++observed;
      });
      auto const expected_random{random_state(file)}; auto const &actual{random.snapshot()};
      if(actual.words != expected_random.words || actual.first != expected_random.first || actual.second != expected_random.second) file.fail(label + " shared effect random state");
      ++frames;
    }
  }
  file.require_end();
  if(traces != 9 || frames != 1620) file.fail("effect-world reference coverage");
  std::cout << frames << " original mixed effect frames agree, including " << observed << " live states, slot reuse, retirement and shared randomness" << std::endl;
  return EXIT_SUCCESS;
} catch(std::exception const &error) {
  std::cerr << error.what() << std::endl;
  return EXIT_FAILURE;
}
