#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include "resources/binary_reader.h"
#include "resources/model.h"
#include "resources/resource_store.h"
#include "simulation/model_instance.h"

namespace {

template<typename T>
vec3<T> read_vector(earthsiege::binary_reader &file) {
  return {file.integer<T>(), file.integer<T>(), file.integer<T>()};
}

void require(bool const condition, unsigned int const row, char const *field) {
  if(!condition) throw std::runtime_error{"model instance differs at reference " + std::to_string(row) + ": " + field};
}

} // anonymous namespace

auto main(int const argc, char **argv)->int try {
  if(argc != 3) return EXIT_FAILURE;
  earthsiege::resource_store resources{argv[1]};
  auto const bytes{earthsiege::read_file(argv[2])};
  earthsiege::binary_reader file{bytes};
  auto const magic{file.take(8)};
  require(std::string(magic.begin(), magic.end()) == "ESPOSE01", 0, "fixture version");
  auto const count{file.integer<uint32_t>()};
  std::map<std::string, earthsiege::model_resource> models;
  for(unsigned int row{0}; row != count; ++row) {
    auto const name{file.cstring()};
    auto const split{name.find('/')};
    if(!models.contains(name)) models.emplace(name, earthsiege::read_model(resources.read(name.substr(0, split), name.substr(split + 1))));
    auto const root{file.integer<uint16_t>()};
    auto const sequence{file.integer<int16_t>()};
    auto const frame{file.integer<int16_t>()};
    auto const next_sequence{file.integer<int16_t>()};
    auto const next_frame{file.integer<int16_t>()};
    auto const fraction{file.integer<int16_t>()};
    auto const &shape{std::get<earthsiege::shape_part>(models.at(name).roots.at(root).content)};
    earthsiege::model_instance instance{shape};
    if(sequence >= 0) {
      instance.apply_frame(static_cast<unsigned int>(sequence), static_cast<unsigned int>(frame), static_cast<unsigned int>(next_sequence), static_cast<unsigned int>(next_frame), fraction);
      instance.update();
    }
    auto const poses{file.integer<uint16_t>()};
    require(instance.poses().size() == poses, row, "pose count");
    for(unsigned int node{0}; node != poses; ++node) {
      require(instance.poses()[node].rotation == read_vector<uint16_t>(file), row, "rotation");
      require(instance.poses()[node].translation == read_vector<int16_t>(file), row, "translation");
    }
    auto const matrices{file.integer<uint16_t>()};
    require(instance.transforms().size() == matrices, row, "matrix count");
    for(unsigned int node{0}; node != matrices; ++node) {
      auto const &transform{instance.transforms()[node]};
      bool const defined{node == 0 || std::ranges::any_of(shape.animation->relations, [node](auto const &relation){ return relation.node == static_cast<int>(node); })};
      for(unsigned int const index : {0U, 1U, 3U, 4U, 2U, 5U, 6U, 7U, 8U}) {
        auto const expected{file.integer<int16_t>()};
        require(!defined || transform.basis.data[index] == expected, row, "matrix basis");
      }
      auto const type{file.integer<uint8_t>()};
      require(!defined || transform.type == type, row, "matrix type");
      file.integer<uint8_t>();                                                  // original unwritten padding
      auto const translation{read_vector<int32_t>(file)};
      require(!defined || transform.translation == translation, row, "matrix translation");
    }
    auto const cells{file.integer<uint16_t>()};
    if(sequence < 0) require(instance.cell_frames().size() == cells, row, "cell count");
    for(unsigned int cell{0}; cell != cells; ++cell) require(instance.cell_frames()[cell] == file.integer<uint16_t>(), row, "default cell frame");
  }
  file.require_end();
  std::cout << count << " original instance defaults and blended skeletal poses agree" << std::endl;
  return EXIT_SUCCESS;
} catch(std::exception const &error) {
  std::cerr << error.what() << std::endl;
  return EXIT_FAILURE;
}
