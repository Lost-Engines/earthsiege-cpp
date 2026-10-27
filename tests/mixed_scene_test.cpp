#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include "render/scenery_renderer.h"
#include "resources/input_tape.h"
#include "resources/player_roster.h"
#include "resources/resource_store.h"
#include "simulation/herc_body.h"

int main(int const argc, char **argv) try {
  if(argc != 3) return EXIT_FAILURE;
  earthsiege::resource_store resources{argv[1]};
  auto const bindings{earthsiege::read_distance_shade_bindings(resources.read("simvol0", "dat/SHADES.DAT"))};
  auto const weapons{earthsiege::read_weapon_catalogue(resources.read("simvol0", "dat/WEAPONS.DAT"))};
  auto const templates{earthsiege::read_model(resources.read("simvol0", "dts/MECHWPNS.DTS"))};
  auto const weapon_textures{earthsiege::read_bitmaps(resources.read("simvol0", "dba/WPNTEX.DBA"))};
  earthsiege::binary_reader names_file{resources.read("simvol0", "nam/MECHS.NAM")};
  std::vector<std::string> names;
  while(names_file.remaining() > 2) names.push_back(names_file.cstring());
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
    earthsiege::mission_header const mission{.terrain{static_cast<earthsiege::terrain_kind>(header[0])}, .zone{header[1]}, .world_variant{header[8]}, .palette_variant{header[9]}};
    earthsiege::scenery_world world{resources, mission};
    earthsiege::framebuffer frame;
    earthsiege::scenery_renderer renderer{frame, world, bindings};
    std::array const lights{earthsiege::default_world_light()};
    std::vector<std::unique_ptr<earthsiege::herc_definition>> definitions;
    std::vector<std::unique_ptr<earthsiege::herc_body>> bodies;
    std::vector<std::vector<earthsiege::model_attachment>> attachments;
    std::vector<earthsiege::scene_model> actors;
    auto const actor_count{file.integer<uint16_t>()};
    attachments.reserve(actor_count);
    for(unsigned int actor{0}; actor != actor_count; ++actor) {
      auto const chassis{file.integer<uint16_t>()};
      auto const native_class{file.integer<int16_t>()};
      vec3<int32_t> const position{file.integer<int32_t>(), file.integer<int32_t>(), file.integer<int32_t>()};
      vec3<uint16_t> const angles{file.integer<uint16_t>(), file.integer<uint16_t>(), file.integer<uint16_t>()};
      definitions.push_back(std::make_unique<earthsiege::herc_definition>(earthsiege::read_herc_definition(resources, names.at(chassis))));
      auto const &definition{*definitions.back()};
      auto const entry{std::ranges::find(roster.hercs, chassis, &earthsiege::player_herc::chassis)};
      if(entry == roster.hercs.end()) file.fail("missing fixture chassis");
      bodies.push_back(std::make_unique<earthsiege::herc_body>(definition, weapons, templates, entry->weapons, position, angles));
      auto const &body{*bodies.back()};
      attachments.emplace_back();
      for(auto const &part : body.weapons().parts()) attachments.back().push_back({.body_part{part.body_part}, .geometry{part.geometry ? &*part.geometry : nullptr}, .cells{part.cell_frames}, .textures{weapon_textures}});
      actors.push_back({.roots{definition.models.roots}, .instance{body.model()}, .placement{body.placement()}, .native_class{native_class}, .textures{definition.textures}, .attachments{attachments.back()}, .detail_thresholds{earthsiege::herc_detail_thresholds}, .detail{}});
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
