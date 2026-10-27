#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include "resources/binary_reader.h"
#include "simulation/effect_state.h"
#include "simulation/object_pool.h"

namespace {

template<typename T> vec3<T> vector(earthsiege::binary_reader &file) { return {file.integer<T>(), file.integer<T>(), file.integer<T>()}; }
earthsiege::debris_particle particle(earthsiege::binary_reader &file) { return {0, vector<int32_t>(file), vector<uint16_t>(file), vector<int16_t>(file)}; }
std::vector<size_t> indices(earthsiege::binary_reader &file) {
  std::vector<size_t> result(file.integer<uint16_t>());
  for(auto &v : result) v = file.integer<uint16_t>();
  return result;
}
void header(earthsiege::binary_reader &file, std::string const &name) {
  auto const magic{file.take(8)}; if(std::string(magic.begin(), magic.end()) != name) file.fail("fixture version");
}

} // namespace

int main(int const argc, char **argv) try {
  if(argc != 3) return EXIT_FAILURE;
  auto const motion_bytes{earthsiege::read_file(argv[1])}; earthsiege::binary_reader file{motion_bytes}; header(file, "ESDEBR01");
  auto const traces{file.integer<uint32_t>()}; unsigned int updates{0};
  for(unsigned int row{0}; row != traces; ++row) {
    file.integer<int16_t>();                                                   // original frame-ratio input intentionally has no effect here
    auto state{particle(file)};
    auto const count{file.integer<uint16_t>()};
    for(unsigned int step{0}; step != count; ++step) {
      auto const expected{particle(file)}; bool const done{file.integer<uint8_t>() != 0};
      if(earthsiege::advance_debris(state) != done || state.position != expected.position || state.angles != expected.angles || state.velocity != expected.velocity) file.fail("debris motion " + std::to_string(row) + '/' + std::to_string(step));
      ++updates;
    }
  }
  file.require_end();
  auto const pool_bytes{earthsiege::read_file(argv[2])}; earthsiege::binary_reader pools{pool_bytes}; header(pools, "ESPOOL01");
  auto const capacities{pools.integer<uint16_t>()}; unsigned int operations{0};
  for(unsigned int group{0}; group != capacities; ++group) {
    earthsiege::object_pool<int> pool{pools.integer<uint16_t>()};
    auto const count{pools.integer<uint16_t>()};
    for(unsigned int step{0}; step != count; ++step) {
      auto const operation{pools.integer<uint8_t>()}; auto const index{pools.integer<int16_t>()}; auto const free_count{pools.integer<uint16_t>()};
      auto const active{indices(pools)}, pending{indices(pools)}, retired{indices(pools)};
      std::vector<size_t> destroyed;
      switch(operation) {
        case 0: {
          auto const result{pool.emplace(static_cast<int>(step))};
          if((result ? static_cast<int>(*result) : -1) != index) pools.fail("pool allocation order");
          break;
        }
        case 1: pool.defer_erase(static_cast<size_t>(index)); break;
        case 2: pool.erase(static_cast<size_t>(index)); break;
        case 3: pool.collect([&](size_t const slot, int &){ destroyed.push_back(slot); }); break;
        default: pools.fail("invalid pool operation");
      }
      std::vector<size_t> actual;
      pool.for_each([&](size_t const slot, int const &){ actual.push_back(slot); });
      if(actual != active || destroyed != retired || pool.available() != free_count || pool.size() != active.size()) pools.fail("pool traversal/reclamation order");
      for(auto const slot : active) {
        bool const expected{std::ranges::find(pending, slot) != pending.end()};
        if(pool.retiring(slot) != expected) pools.fail("deferred pool slot state");
      }
      ++operations;
    }
  }
  pools.require_end();
  if(traces != 960 || updates != 41576 || capacities != 4 || operations != 2400) pools.fail("effect lifetime reference coverage");
  std::cout << updates << " original debris updates and " << operations << " pool allocation/reclamation operations agree" << std::endl;
  return EXIT_SUCCESS;
} catch(std::exception const &error) {
  std::cerr << error.what() << std::endl;
  return EXIT_FAILURE;
}
