#include <cstdlib>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>
#include "resources/resource_store.h"
#include "simulation/model_math.h"
#include "simulation/scenery_collision.h"

namespace {

vec3<int32_t> point(earthsiege::binary_reader &file) {
  return vec3<int32_t>{file.integer<int32_t>(), file.integer<int32_t>(), file.integer<int32_t>()};
}

void require(bool const success, std::string const &name, std::string const &kind, unsigned int const row) {
  if(!success) throw std::runtime_error{name + ' ' + kind + ' ' + std::to_string(row) + " differs"};
}

} // anonymous namespace

int main(int const argc, char **argv) try {
  if(argc != 3) return EXIT_FAILURE;
  earthsiege::resource_store resources{argv[1]};
  auto const bytes{earthsiege::read_file(argv[2])};
  earthsiege::binary_reader file{bytes};
  auto const magic{file.take(8)};
  if(std::string(magic.begin(), magic.end()) != "ESCOLL01") file.fail("fixture version");
  auto const fields{file.integer<uint32_t>()};
  std::map<std::string, earthsiege::model_resource> models;
  unsigned int heights{0}, segments{0}, rays{0}, hits{0};
  for(unsigned int row{0}; row != fields; ++row) {
    auto const name{file.cstring()};
    auto const root{file.integer<uint16_t>()};
    auto const split{name.find('/')};
    if(!models.contains(name)) models.emplace(name, earthsiege::read_model(resources.read(name.substr(0, split), name.substr(split + 1))));
    auto const &model{models.at(name).roots.at(root)};
    auto const &field{*std::get<earthsiege::shape_part>(model.content).collision};
    auto const label{name + '/' + std::to_string(root)};
    auto const height_count{file.integer<uint32_t>()};
    for(unsigned int index{0}; index != height_count; ++index) {
      vec2<int32_t> const at{file.integer<int32_t>(), file.integer<int32_t>()};
      auto const radius{file.integer<int16_t>()};
      auto const expected{file.integer<int32_t>()};
      require(earthsiege::scenery_height(field, at, radius) == expected, label, "height", index);
      ++heights;
    }
    auto const segment_count{file.integer<uint32_t>()};
    for(unsigned int index{0}; index != segment_count; ++index) {
      auto const start{point(file)};
      auto const end{point(file)};
      auto const radius{file.integer<int16_t>()};
      bool const expected_hit{file.integer<uint8_t>() != 0};
      auto const actual{earthsiege::intersect_heightfield(field, start, end, radius)};
      require(actual.has_value() == expected_hit, label, "segment hit", index);
      if(expected_hit) require(*actual == point(file), label, "segment point", index);
      ++segments;
    }
    auto const ray_count{file.integer<uint32_t>()};
    for(unsigned int index{0}; index != ray_count; ++index) {
      auto ray{earthsiege::rotation_transform(vec3<uint16_t>{0, 0, file.integer<uint16_t>()})};
      ray.translation = point(file);
      auto const reach{file.integer<int32_t>()};
      auto const radius{file.integer<int16_t>()};
      auto const count{file.integer<uint16_t>()};
      std::vector<earthsiege::scenery_collider> objects;
      for(unsigned int object{0}; object != count; ++object) {
        auto const position{point(file)};
        auto placement{earthsiege::rotation_transform(vec3<uint16_t>{0, 0, file.integer<uint16_t>()})};
        placement.translation = vec3<int32_t>{position};
        objects.push_back(earthsiege::scenery_collider{.field{&field}, .placement{placement}, .radius{model.header.radius}});
      }
      auto const selected{file.integer<int16_t>()};
      auto const distance{file.integer<int32_t>()};
      auto const actual{earthsiege::intersect_scenery(objects, ray, reach, radius)};
      require(actual.has_value() == (selected >= 0), label, "scene hit", index);
      if(actual) {
        require(actual->object == static_cast<size_t>(selected) && actual->distance == distance, label, "scene selection/distance", index);
        ++hits;
      }
      ++rays;
    }
  }
  file.require_end();
  if(fields != 140 || heights != 38080 || segments != 8960 || rays != 3360 || hits != 1218) file.fail("incomplete collision fixtures");
  std::cout << heights << " original heights, " << segments << " stepped segments and " << rays << " scene rays agree across all " << fields << " collision fields (" << hits << " scene hits)" << std::endl;
  return EXIT_SUCCESS;
} catch(std::exception const &error) {
  std::cerr << error.what() << std::endl;
  return EXIT_FAILURE;
}
