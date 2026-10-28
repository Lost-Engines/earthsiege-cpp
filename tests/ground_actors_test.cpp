#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <string>
#include "resources/input_tape.h"
#include "resources/resource_store.h"
#include "simulation/ground_actor.h"

int main(int const argc, char **argv) try {
  if(argc != 3) return EXIT_FAILURE;
  earthsiege::resource_store resources{argv[1]};
  auto const tape{earthsiege::read_input_tape(earthsiege::read_file(resources.root() / "TAPES/DEMO1.TAP"))};
  auto const script{earthsiege::read_mission_script(tape.starting_files[std::to_underlying(earthsiege::tape_file::script)])};
  // Ground capture registers only its tested actor, with the original world's palette/environment.
  auto header{script.header}; header.zone = -1;
  earthsiege::scenery_world world{resources, header}; earthsiege::ground_catalogue catalogue{resources};
  auto const shading{earthsiege::read_distance_shade_bindings(resources.read("simvol0", "dat/SHADES.DAT"))};
  earthsiege::framebuffer frame; earthsiege::scenery_renderer renderer{frame, world, shading};
  std::array const lights{earthsiege::default_world_light()};
  auto const bytes{earthsiege::read_file(argv[2])}; earthsiege::binary_reader file{bytes};
  auto const magic{file.take(8)};
  if(std::string(magic.begin(), magic.end()) != "ESGBASE1") file.fail("fixture version");
  auto const count{file.integer<uint16_t>()}; unsigned int frames{0}, mismatches{0};
  for(unsigned int row{0}; row != count; ++row) {
    auto const side{file.integer<uint8_t>()}; auto const type{file.integer<int16_t>()}; auto const condition{file.integer<int16_t>()};
    auto const damage{file.integer<int16_t>()}; bool const dead{file.integer<uint8_t>() != 0};
    std::vector<uint16_t> cells(file.integer<uint16_t>()); for(auto &v : cells) v = file.integer<uint16_t>();
    earthsiege::ground_actor actor{catalogue, static_cast<size_t>(type), side, condition, {0, 20000, 0}, {0, 0, 0}};
    auto const label{std::to_string(type) + '/' + std::to_string(side) + '/' + std::to_string(condition)};
    if(actor.dead() != dead || actor.damage() != damage) file.fail(label + " starting condition");
    auto const actual_cells{actor.instance().cell_frames()};
    if(!std::ranges::equal(actual_cells, cells)) file.fail(label + " cell state");
    auto const amount{file.integer<uint16_t>()};
    for(unsigned int angle{0}; angle != amount; ++angle) {
      vec3<int32_t> const position{file.integer<int32_t>(), file.integer<int32_t>(), file.integer<int32_t>()};
      vec3<uint16_t> const angles{file.integer<uint16_t>(), file.integer<uint16_t>(), file.integer<uint16_t>()};
      auto const crc{file.integer<uint32_t>()};
      std::array const actors{actor.model()}; frame.clear(); renderer.draw(position, angles, lights, false, actors);
      if(earthsiege::crc32(frame.pixels()) != crc) {
        if(mismatches++ == 0) frame.save("ground-mismatch.ppm", world.colours());
        std::cerr << label << " angle " << angle << " pixel mismatch\n";
      }
      ++frames;
    }
  }
  file.require_end();
  if(mismatches) file.fail(std::to_string(mismatches) + " ground actor frames differ");
  if(count != 72 || frames != 304) file.fail("ground actor coverage differs");
  std::cout << count << " original base starts and " << frames << " complete frames agree\n";
  return EXIT_SUCCESS;
} catch(std::exception const &error) { std::cerr << error.what() << '\n'; return EXIT_FAILURE; }
