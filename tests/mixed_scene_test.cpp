#include <cstdlib>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include "render/scenery_renderer.h"
#include "resources/input_tape.h"
#include "resources/player_roster.h"
#include "resources/resource_store.h"
#include "simulation/mission_session.h"
#include "simulation/model_math.h"

int main(int const argc, char **argv) try {
  if(argc != 3) return EXIT_FAILURE;
  earthsiege::resource_store resources{argv[1]};
  auto const bindings{earthsiege::read_distance_shade_bindings(resources.read("simvol0", "dat/SHADES.DAT"))};
  auto const bytes{earthsiege::read_file(argv[2])};
  earthsiege::binary_reader file{bytes};
  auto const magic{file.take(8)};
  if(std::string(magic.begin(), magic.end()) != "ESMIXED1") file.fail("fixture version");
  auto const worlds{file.integer<uint16_t>()};
  unsigned int failed{0}, total{0};
  for(unsigned int world_index{0}; world_index != worlds; ++world_index) {
    auto const demo{file.cstring()};
    auto const tape{earthsiege::read_input_tape(earthsiege::read_file(resources.root() / "TAPES" / (demo + ".TAP")))};
    auto const roster{earthsiege::read_player_roster(tape.starting_files[std::to_underlying(earthsiege::tape_file::player)])};
    std::array<int16_t, 10> header;
    for(auto &value : header) value = file.integer<int16_t>();
    auto const script{earthsiege::read_mission_script(tape.starting_files[std::to_underlying(earthsiege::tape_file::script)])};
    if(static_cast<int16_t>(script.header.terrain) != header[0] || script.header.zone != header[1] || script.header.world_variant != header[8] || script.header.palette_variant != header[9]) file.fail("mission world differs");
    earthsiege::mission_session session{resources, script, roster};
    auto const &world{session.scenery()};
    earthsiege::framebuffer frame;
    earthsiege::scenery_renderer renderer{frame, world, bindings};
    std::array const lights{earthsiege::default_world_light()};
    auto const actors{session.roster_models()};
    auto const actor_count{file.integer<uint16_t>()};
    if(actor_count != actors.size()) file.fail("roster actor count differs");
    for(unsigned int actor{0}; actor != actor_count; ++actor) {
      file.integer<uint16_t>();                                               // chassis already selects its original resources through the roster
      if(actors[actor].native_class != file.integer<int16_t>()) file.fail("roster class differs");
      vec3<int32_t> const position{file.integer<int32_t>(), file.integer<int32_t>(), file.integer<int32_t>()};
      vec3<uint16_t> const angles{file.integer<uint16_t>(), file.integer<uint16_t>(), file.integer<uint16_t>()};
      if(actors[actor].placement.translation != position) file.fail("roster placement differs");
      if(actors[actor].placement.basis != earthsiege::rotation_transform(angles).basis) file.fail("roster orientation differs");
    }
    auto const count{file.integer<uint16_t>()};
    for(unsigned int row{0}; row != count; ++row) {
      vec3<int32_t> const position{file.integer<int32_t>(), file.integer<int32_t>(), file.integer<int32_t>()};
      vec3<uint16_t> const angles{file.integer<uint16_t>(), file.integer<uint16_t>(), file.integer<uint16_t>()};
      auto const expected{file.integer<uint32_t>()};
      std::vector<size_t> order(file.integer<uint16_t>());
      for(auto &value : order) value = file.integer<uint16_t>();
      frame.clear();
      auto const actual_order{renderer.draw(position, angles, lights, false, actors)};
      bool const order_matches{order == actual_order};
      bool const pixels_match{earthsiege::crc32(frame.pixels()) == expected};
      if(!order_matches || !pixels_match) {
        ++failed;
        std::cerr << demo << " frame " << row << " order " << order_matches << " pixels " << pixels_match << '\n';
        if(failed == 1) {
          frame.save("mixed-mismatch.ppm", world.colours());
          std::ofstream raw{"mixed-mismatch.bin", std::ios::binary};
          raw.write(reinterpret_cast<char const *>(frame.pixels().data()), static_cast<std::streamsize>(frame.pixels().size()));
        }
      }
      ++total;
    }
  }
  file.require_end();
  if(failed) throw std::runtime_error{std::to_string(failed) + " complete mixed scenes differ"};
  std::cout << total << " complete original mixed scene frames and painter orders agree" << std::endl;
  return EXIT_SUCCESS;
} catch(std::exception const &error) {
  std::cerr << error.what() << std::endl;
  return EXIT_FAILURE;
}
