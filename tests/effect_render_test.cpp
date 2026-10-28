#include <cstdlib>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include "resources/resource_store.h"
#include "simulation/world_effects.h"

namespace {

template<typename T> vec3<T> vector(earthsiege::binary_reader &file) { return {file.integer<T>(), file.integer<T>(), file.integer<T>()}; }
earthsiege::random_state::state random(earthsiege::binary_reader &file) {
  earthsiege::random_state::state result;
  for(auto &word : result.words) word = file.integer<uint16_t>();
  result.first = file.integer<uint8_t>(); result.second = file.integer<uint8_t>();
  return result;
}

} // anonymous namespace

int main(int const argc, char **argv) try {
  if(argc != 3) return EXIT_FAILURE;
  earthsiege::resource_store resources{argv[1]};
  auto const bytes{earthsiege::read_file(argv[2])}; earthsiege::binary_reader file{bytes};
  auto const magic{file.take(8)};
  if(std::string(magic.begin(), magic.end()) != "ESFXRN01") file.fail("fixture version");
  std::array<int16_t, 10> header;
  for(auto &value : header) value = file.integer<int16_t>();
  earthsiege::mission_header const mission{.terrain{static_cast<earthsiege::terrain_kind>(header[0])}, .zone{header[1]}, .world_variant{header[8]}, .palette_variant{header[9]}};
  auto const bindings{earthsiege::read_distance_shade_bindings(resources.read("simvol0", "dat/SHADES.DAT"))};
  size_t frames{0}, failed{0};
  auto const cases{file.integer<uint16_t>()};
  for(size_t row{0}; row != cases; ++row) {
    earthsiege::scenery_world terrain{resources, mission};
    earthsiege::world_effects effects{resources};
    earthsiege::framebuffer frame;
    earthsiege::scenery_renderer renderer{frame, terrain, bindings};
    earthsiege::random_state rng{random(file)};
    auto const requests{file.integer<uint16_t>()};
    for(size_t index{0}; index != requests; ++index) {
      auto const kind{file.integer<int32_t>()}, selected{file.integer<int32_t>()};
      auto const position{vector<int32_t>(file)};
      if(kind == 0) effects.add_explosion(static_cast<int16_t>(selected), position, rng);
      else effects.add_debris(static_cast<int16_t>(selected), position, rng);
    }
    auto const count{file.integer<uint16_t>()};
    for(size_t index{0}; index != count; ++index, ++frames) {
      auto const updates{file.integer<uint8_t>()};
      for(size_t step{0}; step != updates; ++step) { effects.advance(256); effects.collect(); }
      auto const position{vector<int32_t>(file)}; auto const angles{vector<uint16_t>(file)};
      auto const expected{file.integer<uint32_t>()};
      std::vector<earthsiege::scene_model> models;
      effects.append_models(models);
      auto const ground{effects.ground_flashes()};
      frame.clear();
      renderer.draw(position, angles, {}, false, models, &effects.lights(), ground);
      auto const actual{earthsiege::crc32(frame.pixels())};
      if(actual != expected) {
        std::cerr << "effect frame " << row << '/' << index << " actual " << actual << " expected " << expected << '\n';
        if(failed++ == 0) {
          frame.save("effect-mismatch.ppm", terrain.colours());
          std::ofstream raw{"effect-mismatch.bin", std::ios::binary};
          raw.write(reinterpret_cast<char const *>(frame.pixels().data()), static_cast<std::streamsize>(frame.pixels().size()));
        }
      }
    }
  }
  file.require_end();
  if(failed) throw std::runtime_error{std::to_string(failed) + " effect/scenery drawings differ"};
  std::cout << frames << " complete original effect/scenery frames match every pixel\n";
  return EXIT_SUCCESS;
} catch(std::exception const &error) {
  std::cerr << error.what() << '\n';
  return EXIT_FAILURE;
}
