#include <cstdlib>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include "render/model_renderer.h"
#include "resources/resource_store.h"
#include "simulation/model_math.h"
#include "simulation/weapon_assembly.h"

int main(int const argc, char **argv) try {
  if(argc != 3) return EXIT_FAILURE;
  earthsiege::resource_store resources{argv[1]};
  auto const definitions{earthsiege::read_weapon_catalogue(resources.read("simvol0", "dat/WEAPONS.DAT"))};
  auto const templates{earthsiege::read_model(resources.read("simvol0", "dts/MECHWPNS.DTS"))};
  auto const weapon_textures{earthsiege::read_bitmaps(resources.read("simvol0", "dba/WPNTEX.DBA"))};
  auto const bytes{earthsiege::read_file(argv[2])};
  earthsiege::binary_reader file{bytes};
  auto const magic{file.take(8)};
  if(std::string(magic.begin(), magic.end()) != "ESARMED1") file.fail("fixture version");
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
    std::array<int16_t, 5> pose;
    for(auto &value : pose) value = file.integer<int16_t>();
    auto const owner{earthsiege::read_model(resources.read("simvol0", "dts/" + name + ".DTS"))};
    auto const mounts{earthsiege::read_weapon_mounts(resources.read("simvol0", "gl/" + name + ".GL"))};
    earthsiege::weapon_assembly assembly{owner.roots.front(), mounts, equipment, definitions, templates};
    std::vector<earthsiege::model_attachment> attachments;
    for(auto const &part : assembly.parts()) attachments.push_back({.body_part{part.body_part}, .geometry{part.geometry ? &*part.geometry : nullptr}, .cells{part.cell_frames}, .textures{weapon_textures}});
    earthsiege::model_instance instance{std::get<earthsiege::shape_part>(owner.roots.front().content)};
    if(pose[0] >= 0) {
      instance.apply_frame(static_cast<unsigned int>(pose[0]), static_cast<unsigned int>(pose[1]), static_cast<unsigned int>(pose[2]), static_cast<unsigned int>(pose[3]), pose[4]);
      instance.update();
    }
    auto const colours{earthsiege::read_palette(resource(palette_name))};
    auto const textures{earthsiege::read_bitmaps(resource(texture_name))};
    auto object{earthsiege::rotation_transform(angles)};
    object.translation = translation;
    std::array const lights{earthsiege::default_world_light()};
    earthsiege::framebuffer frame;
    earthsiege::model_renderer renderer{frame, colours};
    renderer.draw(owner.roots.front(), instance, object, {}, textures, lights, attachments);
    if(earthsiege::crc32(frame.pixels()) != expected) {
      ++failed;
      std::cerr << "case " << row << ' ' << name << " differs: " << earthsiege::crc32(frame.pixels()) << " expected " << expected << '\n';
      if(failed == 1) {
        frame.save("armed-mismatch.ppm", colours);
        std::ofstream raw{"armed-mismatch.bin", std::ios::binary};
        raw.write(reinterpret_cast<char const *>(frame.pixels().data()), static_cast<std::streamsize>(frame.pixels().size()));
      }
    }
  }
  file.require_end();
  if(failed) throw std::runtime_error{std::to_string(failed) + " armed frames differ"};
  std::cout << count << " complete original armed/unarmed HERC frames match all 64,000 pixels" << std::endl;
  return EXIT_SUCCESS;
} catch(std::exception const &error) {
  std::cerr << error.what() << std::endl;
  return EXIT_FAILURE;
}
