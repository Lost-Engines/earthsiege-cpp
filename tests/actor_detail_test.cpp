#include <cstdlib>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include "render/model_renderer.h"
#include "resources/resource_store.h"
#include "resources/herc_profile.h"
#include "resources/model_bindings.h"
#include "simulation/model_math.h"
#include "simulation/weapon_assembly.h"

int main(int const argc, char **argv) try {
  if(argc != 3 && argc != 4) return EXIT_FAILURE;
  earthsiege::resource_store resources{argv[1]};
  auto const definitions{earthsiege::read_weapon_catalogue(resources.read("simvol0", "dat/WEAPONS.DAT"))};
  auto const templates{earthsiege::read_model(resources.read("simvol0", "dts/MECHWPNS.DTS"))};
  auto const weapon_textures{earthsiege::read_bitmaps(resources.read("simvol0", "dba/WPNTEX.DBA"))};
  auto const bytes{earthsiege::read_file(argv[2])};
  earthsiege::binary_reader file{bytes};
  auto const magic{file.take(8)};
  if(std::string(magic.begin(), magic.end()) != (argc==4?"ESSHPFR1":"ESDETAIL")) file.fail("fixture version");
  auto const count{file.integer<uint32_t>()};
  auto const resource{[&](std::string const &name){
    auto const slash{name.find('/')};
    return resources.read(name.substr(0, slash), name.substr(slash + 1));
  }};
  unsigned int failed{0};
  for(unsigned int row{0}; row != count; ++row) {
    auto const name{file.cstring()};
    auto const texture_name{file.cstring()};
    auto const palette_name{file.cstring()};
    std::vector<int16_t> equipment(file.integer<uint16_t>());
    for(auto &value : equipment) value = file.integer<int16_t>();
    vec3<uint16_t> const angles{file.integer<uint16_t>(), file.integer<uint16_t>(), file.integer<uint16_t>()};
    vec3<int32_t> const translation{file.integer<int32_t>(), file.integer<int32_t>(), file.integer<int32_t>()};
    auto const expected{file.integer<uint32_t>()};
    earthsiege::model_detail detail;
    detail.scale = file.integer<int16_t>();
    detail.bias = file.integer<uint16_t>();
    auto const expected_detail{file.integer<uint16_t>()};
    earthsiege::model_view view;
    if(argc==4){view.detail.bias=file.integer<uint8_t>();view.weapon_detail_bias=file.integer<uint8_t>();}
    auto owner{earthsiege::read_model(resources.read("simvol0", "dts/" + name + ".DTS"))};
    auto const profile{earthsiege::read_herc_profile(resources.read("simvol0", "dat/" + name + ".DAT"))};
    earthsiege::bind_herc_detail_nodes(owner, profile.detail_part_ids);
    auto const mounts{earthsiege::read_weapon_mounts(resources.read("simvol0", "gl/" + name + ".GL"))};
    earthsiege::weapon_assembly assembly{owner.roots.front(), mounts, equipment, definitions, templates};
    std::vector<earthsiege::model_attachment> attachments;
    for(auto const &part : assembly.parts()) attachments.push_back({.body_part{part.body_part}, .geometry{part.geometry ? &*part.geometry : nullptr}, .cells{part.cell_frames}, .textures{weapon_textures}});
    earthsiege::model_instance instance{std::get<earthsiege::shape_part>(owner.roots.front().content)};
    auto const colours{earthsiege::read_palette(resource(palette_name))};
    auto const textures{earthsiege::read_bitmaps(resource(texture_name))};
    auto object{earthsiege::rotation_transform(angles)};
    object.translation = translation;
    std::array const lights{earthsiege::default_world_light()};
    earthsiege::framebuffer frame;
    earthsiege::model_renderer renderer{frame, colours};
    auto const eye{earthsiege::inverse_transform(object).translation};
    auto const selected{earthsiege::actor_detail_index(owner.roots.size(), earthsiege::herc_detail_thresholds, owner.roots.front().header.radius, eye, detail)};
    if(selected != expected_detail) throw std::runtime_error{name + " detail differs in case " + std::to_string(row)};
    renderer.draw(owner.roots.at(selected), instance, object, view, textures, lights, attachments);
    if(earthsiege::crc32(frame.pixels()) != expected) {
      ++failed;
      std::cerr << "case " << row << ' ' << name << " differs: " << earthsiege::crc32(frame.pixels()) << " expected " << expected << '\n';
      if(failed == 1) {
        frame.save("detail-mismatch.ppm", colours);
        std::ofstream raw{"detail-mismatch.bin", std::ios::binary};
        raw.write(reinterpret_cast<char const *>(frame.pixels().data()), static_cast<std::streamsize>(frame.pixels().size()));
      }
    }
  }
  file.require_end();
  if(failed) throw std::runtime_error{std::to_string(failed) + " actor detail frames differ"};
  std::cout << count << " original actor detail choices and complete frames match all 64,000 pixels" << std::endl;
  return EXIT_SUCCESS;
} catch(std::exception const &error) {
  std::cerr << error.what() << std::endl;
  return EXIT_FAILURE;
}
