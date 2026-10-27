#include <array>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include "render/model_renderer.h"
#include "resources/resource_store.h"
#include "simulation/herc_actor.h"
#include "simulation/model_math.h"

int main(int const argc, char **argv) try {
  if(argc != 3) return EXIT_FAILURE;
  earthsiege::resource_store resources{argv[1]};
  auto const weapons{earthsiege::read_weapon_catalogue(resources.read("simvol0", "dat/WEAPONS.DAT"))};
  auto const templates{earthsiege::read_model(resources.read("simvol0", "dts/MECHWPNS.DTS"))};
  auto const weapon_textures{earthsiege::read_bitmaps(resources.read("simvol0", "dba/WPNTEX.DBA"))};
  auto const bytes{earthsiege::read_file(argv[2])};
  earthsiege::binary_reader file{bytes};
  auto const magic{file.take(8)};
  if(std::string(magic.begin(), magic.end()) != "ESDMGR01") file.fail("fixture version");
  auto const count{file.integer<uint32_t>()};
  for(unsigned int row{0}; row != count; ++row) {
    auto const name{file.cstring()};
    auto const palette_name{file.cstring()};
    std::vector<int16_t> equipment(file.integer<uint16_t>());
    for(auto &value : equipment) value = file.integer<int16_t>();
    vec3<uint16_t> const angles{file.integer<uint16_t>(), file.integer<uint16_t>(), file.integer<uint16_t>()};
    vec3<int32_t> const translation{file.integer<int32_t>(), file.integer<int32_t>(), file.integer<int32_t>()};
    auto const expected{file.integer<uint32_t>()};
    auto const section{file.integer<int16_t>()};
    auto const amount{file.integer<int16_t>()};
    std::vector<std::array<int16_t, 2>> draws(file.integer<uint16_t>());
    for(auto &draw : draws) draw = {file.integer<int16_t>(), file.integer<int16_t>()};
    auto const definition{earthsiege::read_herc_definition(resources, name)};
    earthsiege::herc_actor actor{definition, weapons, templates, equipment, translation, angles};
    size_t consumed{0};
    actor.damage_section(static_cast<size_t>(section), amount, [&](int16_t const bound){
      if(consumed == draws.size() || bound != draws[consumed][0]) file.fail("damage choice differs");
      return draws[consumed++][1];
    });
    if(consumed != draws.size()) file.fail("unconsumed damage choices");
    std::vector<earthsiege::model_attachment> attachments;
    for(auto const &part : actor.body().weapons().parts()) attachments.push_back({.body_part{part.body_part}, .geometry{part.geometry && !part.disabled ? &*part.geometry : nullptr}, .cells{part.cell_frames}, .textures{weapon_textures}});
    auto const slash{palette_name.find('/')};
    auto const colours{earthsiege::read_palette(resources.read(palette_name.substr(0, slash), palette_name.substr(slash + 1)))};
    std::array const lights{earthsiege::default_world_light()};
    earthsiege::framebuffer frame;
    earthsiege::model_renderer renderer{frame, colours};
    renderer.draw(definition.models.roots.front(), actor.body().model(), actor.body().placement(), {}, definition.textures, lights, attachments);
    if(earthsiege::crc32(frame.pixels()) != expected) {
      frame.save("damaged-mismatch.ppm", colours);
      file.fail("damaged frame " + std::to_string(row) + ' ' + name + " differs");
    }
  }
  file.require_end();
  if(count == 0) file.fail("no damaged frames");
  std::cout << count << " original post-impact HERC frames match all 64,000 pixels" << std::endl;
  return EXIT_SUCCESS;
} catch(std::exception const &error) {
  std::cerr << error.what() << std::endl;
  return EXIT_FAILURE;
}
