#include <algorithm>
#include <array>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include "render/model_traversal.h"
#include "resources/model.h"
#include "resources/resource_store.h"
#include "simulation/model_math.h"

namespace {

auto part_at(earthsiege::model_resource const &model, std::string const &source)->earthsiege::model_part const & {
  std::string path{source};
  std::ranges::replace(path, '.', ' ');
  std::istringstream stream{path};
  unsigned int index{};
  stream >> index;
  auto const *part{&model.roots.at(index)};
  while(stream >> index) {
    part = std::visit([&](auto const &content)->earthsiege::model_part const * {
      if constexpr(requires { content.children; }) return &content.children.at(index);
      else throw std::runtime_error{"invalid fixture path"};
    }, part->content);
  }
  return *part;
}

} // namespace

auto main(int argc, char **argv)->int try {
  if(argc != 3) return EXIT_FAILURE;
  earthsiege::resource_store const resources{argv[1]};
  std::ifstream input{argv[2]};
  if(!input) throw std::runtime_error{"missing native traversal fixture"};
  std::map<std::string, earthsiege::model_resource> models;
  std::array<std::vector<earthsiege::model_transform>, 12> transforms;
  std::array<std::array<uint16_t, 64>, 12> frames{};
  for(unsigned int state{}; state < transforms.size(); ++state) {
    for(unsigned int index{}; index < 64; ++index) {
      vec3<uint16_t> angles;
      if(state & 1) angles = vec3<uint16_t>{
        static_cast<uint16_t>((index + 1) * 733 + state * 271),
        static_cast<uint16_t>((index + 1) * 1879 + state * 271),
        static_cast<uint16_t>((index + 1) * 3571 + state * 271),
      };
      auto transform{earthsiege::rotation_transform(angles)};
      int const node{static_cast<int>(index)};
      transform.translation = vec3<int32_t>{node * 37, -node * 51, node * 79};
      transforms[state].push_back(std::move(transform));
      frames[state][index] = static_cast<uint16_t>(state * 11 + index * 7);
    }
  }
  std::string volume, name, path;
  unsigned int state{}, kind{}, size{}, count{};
  int32_t x{}, y{}, z{};
  while(input >> volume >> name >> path >> state >> kind >> x >> y >> z >> size) {
    std::vector<uint16_t> expected(size);
    for(auto &value : expected) if(!(input >> value)) throw std::runtime_error{"truncated traversal fixture"};
    std::string const key{volume + ':' + name};
    if(!models.contains(key)) models.emplace(key, earthsiege::read_model(resources.read(volume, name)));
    auto const &part{part_at(models.at(key), path)};
    if(std::to_underlying(part.kind) != kind) throw std::runtime_error{"traversal fixture tag mismatch"};
    vec3<int32_t> const eye{x, y, z};
    std::vector<uint16_t> actual;
    if(auto const *bsp{std::get_if<earthsiege::bsp_part>(&part.content)}) {
      actual = earthsiege::bsp_draw_order(*bsp, eye, transforms.at(state));
    } else if(auto const *cell{std::get_if<earthsiege::cell_part>(&part.content)}) {
      actual.push_back(static_cast<uint16_t>(earthsiege::cell_draw_index(*cell, frames.at(state))));
    } else {
      auto const &detail{std::get<earthsiege::detail_part>(part.content)};
      earthsiege::model_detail const settings{
        .projection_shift{6 + state % 4},
        .scale{static_cast<int16_t>(512 + state * 256)},
        .bias{state % std::min(static_cast<unsigned int>(detail.children.size()), 3U)},
      };
      actual.push_back(static_cast<uint16_t>(earthsiege::detail_draw_index(detail, part.header.radius, eye, settings)));
    }
    if(actual != expected) throw std::runtime_error{"original draw selection differs at " + key + " part " + path + " case " + std::to_string(state)};
    ++count;
  }
  if(!input.eof() || count != 28632) throw std::runtime_error{"incomplete traversal comparisons"};
  std::cout << count << " original BSP painter orders, cell frames and detail selections agree\n";
  return EXIT_SUCCESS;
} catch(std::exception const &error) {
  std::cerr << error.what() << '\n';
  return EXIT_FAILURE;
}
