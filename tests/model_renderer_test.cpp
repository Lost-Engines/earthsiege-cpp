#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include "render/model_renderer.h"
#include "resources/binary_reader.h"
#include "resources/resource_store.h"
#include "simulation/model_math.h"

int main(int const argc, char **argv) try {
  if(argc != 3) return EXIT_FAILURE;
  earthsiege::resource_store resources{argv[1]};
  auto const bytes{earthsiege::read_file(argv[2])};
  earthsiege::binary_reader file{bytes};
  auto const magic{file.take(8)};
  bool const multiple_lights{std::string(magic.begin(),magic.end())=="ESMODEL3"};
  if(!multiple_lights&&std::string(magic.begin(), magic.end()) != "ESMODEL2") file.fail("fixture version");
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
    auto const root{file.integer<uint16_t>()};
    vec3<uint16_t> const angles{file.integer<uint16_t>(), file.integer<uint16_t>(), file.integer<uint16_t>()};
    vec3<int32_t> const translation{file.integer<int32_t>(), file.integer<int32_t>(), file.integer<int32_t>()};
    auto const expected{file.integer<uint32_t>()};
    std::array<int16_t, 5> pose;
    for(auto &value : pose) value = file.integer<int16_t>();
    std::vector<earthsiege::light> lights{earthsiege::default_world_light()};
    if(multiple_lights){
      lights.resize(file.integer<uint16_t>());
      for(auto &value:lights){
        value.kind=static_cast<earthsiege::light_kind>(file.integer<uint8_t>());
        value.intensity=file.integer<int32_t>();
        for(unsigned axis{};axis<3;++axis)value.position[axis]=file.integer<int32_t>();
        for(unsigned axis{};axis<3;++axis)value.direction[axis]=file.integer<int16_t>();
        value.attenuation=file.integer<int32_t>();value.scale=file.integer<int32_t>();
      }
    }
    if(!models.contains(name)) models.emplace(name, earthsiege::read_model(resource(name)));
    auto const &part{models.at(name).roots.at(root)};
    earthsiege::model_instance instance{std::get<earthsiege::shape_part>(part.content)};
    if(pose[0] >= 0) {
      instance.apply_frame(static_cast<unsigned int>(pose[0]), static_cast<unsigned int>(pose[1]), static_cast<unsigned int>(pose[2]), static_cast<unsigned int>(pose[3]), pose[4]);
      instance.update();
    }
    auto const colours{earthsiege::read_palette(resource(palette_name))};
    auto const textures{texture_name.empty() ? std::vector<earthsiege::bitmap>{} : earthsiege::read_bitmaps(resource(texture_name))};
    auto object{earthsiege::rotation_transform(angles)};
    object.translation = translation;
    earthsiege::framebuffer frame;
    earthsiege::model_renderer renderer{frame, colours};
    try {
      renderer.draw(part, instance, object, {}, textures, lights);
    } catch(std::exception const &error) {
      throw std::runtime_error{"case " + std::to_string(row) + " " + name + ": " + error.what()};
    }
    if(earthsiege::crc32(frame.pixels()) != expected) {
      ++failed;
      std::cerr << "case " << row << ' ' << name << " root " << root << " differs: " << earthsiege::crc32(frame.pixels()) << " expected " << expected << '\n';
      if(failed == 1) {
        frame.save("model-mismatch.ppm", colours);
        std::ofstream raw{"model-mismatch.bin", std::ios::binary};
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
