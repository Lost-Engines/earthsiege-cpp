#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include "render/model_renderer.h"
#include "resources/cockpit.h"
#include "resources/binary_reader.h"
#include "resources/resource_store.h"
#include "simulation/model_math.h"

int main(int const argc, char **argv) try {
  if(argc != 3) return EXIT_FAILURE;
  earthsiege::resource_store resources{argv[1]};
  auto const bytes{earthsiege::read_file(argv[2])};
  earthsiege::binary_reader file{bytes};
  auto const magic{file.take(8)};
  if(std::string(magic.begin(), magic.end()) != "ESMASK01") file.fail("fixture version");
  auto const count{file.integer<uint32_t>()};
  std::map<std::string, earthsiege::model_resource> models;
  auto const resource{[&](std::string const &name){
    auto const slash{name.find('/')};
    return resources.read(name.substr(0, slash), name.substr(slash + 1));
  }};
  unsigned int failed{0};
  for(unsigned int row{0}; row != count; ++row) {
    auto const name{file.cstring()};
    auto const texture_name{file.cstring()};
    auto const palette_name{file.cstring()};
    auto const mask_name{file.cstring()};
    auto const mask{earthsiege::read_screen_mask(resource(mask_name))};
    auto const root{file.integer<uint16_t>()};
    vec3<uint16_t> const angles{file.integer<uint16_t>(), file.integer<uint16_t>(), file.integer<uint16_t>()};
    vec3<int32_t> const translation{file.integer<int32_t>(), file.integer<int32_t>(), file.integer<int32_t>()};
    auto const expected{file.integer<uint32_t>()};

    if(!models.contains(name)) models.emplace(name, earthsiege::read_model(resource(name)));
    auto const &part{models.at(name).roots.at(root)};
    earthsiege::model_instance instance{std::get<earthsiege::shape_part>(part.content)};

    auto const colours{earthsiege::read_palette(resource(palette_name))};
    auto const textures{texture_name.empty() ? std::vector<earthsiege::bitmap>{} : earthsiege::read_bitmaps(resource(texture_name))};
    auto object{earthsiege::rotation_transform(angles)};
    object.translation = translation;
    std::array const lights{earthsiege::default_world_light()};
    earthsiege::framebuffer frame;
    earthsiege::model_renderer renderer{frame, colours};
    earthsiege::scoped_framebuffer_clip clip{frame, &mask};
    try {
      renderer.draw(part, instance, object, {}, textures, lights);
    } catch(std::exception const &error) {
      throw std::runtime_error{"case " + std::to_string(row) + " " + name + ": " + error.what()};
    }
    if(earthsiege::crc32(frame.pixels()) != expected) {
      ++failed;
      std::cerr << "case " << row << ' ' << name << " " << mask_name << " root " << root << " differs: " << earthsiege::crc32(frame.pixels()) << " expected " << expected << '\n';
      if(failed == 1) {
        frame.save("mask-mismatch.ppm", colours);
        std::ofstream raw{"mask-mismatch.bin", std::ios::binary};
        raw.write(reinterpret_cast<char const *>(frame.pixels().data()), static_cast<std::streamsize>(frame.pixels().size()));
      }
    }
  }
  file.require_end();
  if(failed) throw std::runtime_error{std::to_string(failed) + " complete model renders differ"};
  std::cout << count << " complete original model frames match all 64,000 pixels" << std::endl;
  return EXIT_SUCCESS;
} catch(std::exception const &error) {
  std::cerr << error.what() << std::endl;
  return EXIT_FAILURE;
}
