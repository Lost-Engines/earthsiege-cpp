#include <cstdlib>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include "render/scenery_renderer.h"
#include "resources/resource_store.h"
#include "simulation/beam_effect.h"

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
  if(std::string(magic.begin(), magic.end()) != "ESBEAM01") file.fail("fixture version");
  std::array<int16_t, 10> header;
  for(auto &value : header) value = file.integer<int16_t>();
  earthsiege::mission_header const mission{.terrain{static_cast<earthsiege::terrain_kind>(header[0])}, .zone{header[1]}, .world_variant{header[8]}, .palette_variant{header[9]}};
  auto const bindings{earthsiege::read_distance_shade_bindings(resources.read("simvol0", "dat/SHADES.DAT"))};
  earthsiege::scenery_world terrain{resources, mission};
  std::array const lights{earthsiege::default_world_light()};
  auto const count{file.integer<uint16_t>()}; size_t failed{0};
  for(size_t row{0}; row != count; ++row) {
    auto const subtype{file.integer<int16_t>()};
    earthsiege::beam_path const path{vector<int32_t>(file), vector<int32_t>(file)};
    auto const colour{file.integer<uint8_t>()};
    earthsiege::random_state rng{random(file)};
    auto const expected_random{random(file)};
    auto beam{earthsiege::make_beam_effect(subtype, path, rng)};
    auto const &state{rng.snapshot()};
    auto const require{[&](bool const value, char const *field){ if(!value) file.fail("beam " + std::to_string(row) + ": " + field); }};
    require(beam.position == path.start && beam.colour == colour, "position/colour");
    require(state.words == expected_random.words && state.first == expected_random.first && state.second == expected_random.second, "random state");
    require(beam.points.size() == file.integer<uint16_t>(), "vertex count");
    for(auto const &point : beam.points) require(point == vector<int32_t>(file), "vertex");
    require(beam.faces.size() == file.integer<uint16_t>(), "face count");
    for(auto const &face : beam.faces) for(auto const vertex : face) require(vertex == file.integer<uint16_t>(), "face index");
    bool const expired{file.integer<uint8_t>() != 0};
    auto const position{vector<int32_t>(file)}; auto const angles{vector<uint16_t>(file)};
    auto const expected{file.integer<uint32_t>()};
    earthsiege::framebuffer frame;
    earthsiege::scenery_renderer renderer{frame, terrain, bindings};
    std::array<earthsiege::scene_object, 1> const visible{earthsiege::scene_ribbon{vec3<int32_t>{beam.position}, beam.points, beam.faces, beam.colour}};
    renderer.draw_objects(position, angles, lights, false, visible);
    auto const actual{earthsiege::crc32(frame.pixels())};
    if(actual != expected) {
      std::cerr << "beam drawing " << row << " actual " << actual << " expected " << expected << '\n';
      if(failed++ == 0) {
        frame.save("beam-mismatch.ppm", terrain.colours());
        std::ofstream raw{"beam-mismatch.bin", std::ios::binary};
        raw.write(reinterpret_cast<char const *>(frame.pixels().data()), static_cast<std::streamsize>(frame.pixels().size()));
      }
    }
    require(earthsiege::advance_beam_effect(beam, 256) == expired, "expiry");
  }
  file.require_end();
  if(failed) throw std::runtime_error{std::to_string(failed) + " beam drawings differ"};
  std::cout << count << " original beam shapes, random states, lifetimes and complete drawings agree\n";
  return EXIT_SUCCESS;
} catch(std::exception const &error) {
  std::cerr << error.what() << '\n';
  return EXIT_FAILURE;
}
