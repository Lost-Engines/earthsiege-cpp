#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include "resources/model_tree.h"
#include "resources/resource_store.h"
#include "simulation/weapon_assembly.h"

int main(int const argc, char **argv) try {
  if(argc != 3) return EXIT_FAILURE;
  earthsiege::resource_store resources{argv[1]};
  auto const definitions{earthsiege::read_weapon_catalogue(resources.read("simvol0", "dat/WEAPONS.DAT"))};
  auto const templates{earthsiege::read_model(resources.read("simvol0", "dts/MECHWPNS.DTS"))};
  auto const bytes{earthsiege::read_file(argv[2])};
  earthsiege::binary_reader file{bytes};
  auto const magic{file.take(8)};
  if(std::string(magic.begin(), magic.end()) != "ESWPN001") file.fail("fixture version");
  auto const count{file.integer<uint16_t>()};
  unsigned int weapons{0}, nodes{0}, points{0};
  for(unsigned int row{0}; row != count; ++row) {
    auto const name{file.cstring()};
    auto const require{[&](bool const pass, std::string const &field){
      if(!pass) throw std::runtime_error{name + ": " + field + " differs"};
    }};
    std::vector<int16_t> equipment(file.integer<uint16_t>());
    for(auto &value : equipment) value = file.integer<int16_t>();
    auto const owner{earthsiege::read_model(resources.read("simvol0", "dts/" + name + ".DTS"))};
    auto const mounts{earthsiege::read_weapon_mounts(resources.read("simvol0", "gl/" + name + ".GL"))};
    earthsiege::weapon_assembly assembly{owner.roots.front(), mounts, equipment, definitions, templates};
    require(assembly.parts().size() == static_cast<size_t>(std::ranges::count_if(mounts, [](auto const &mount){ return mount.external(); })), "external mount count");
    auto const mounted{file.integer<uint16_t>()};
    for(unsigned int index{0}; index != mounted; ++index) {
      auto const body_part{file.integer<int16_t>()};
      auto const weapon{file.integer<int16_t>()};
      auto const found{std::ranges::find(assembly.parts(), body_part, &earthsiege::mounted_weapon_model::body_part)};
      require(found != assembly.parts().end() && found->weapon == weapon && found->geometry.has_value(), "mounted weapon");
      require(std::ranges::all_of(found->cell_frames, [](auto const value){ return value == 0; }), "initial weapon cells");
      auto const node_count{file.integer<uint16_t>()};
      for(unsigned int node{0}; node != node_count; ++node) {
        auto const depth{file.integer<uint16_t>()};
        auto const *part{&*found->geometry};
        for(unsigned int level{0}; level != depth; ++level) part = &earthsiege::model_children(*part)[file.integer<uint16_t>()];
        require(part->header.transform == file.integer<int16_t>(), "parent transform");
        require(part->header.id == file.integer<int16_t>(), "part id");
        require(part->header.radius == file.integer<int16_t>(), "radius");
        for(unsigned int axis{0}; axis != 3; ++axis) require(part->header.centre[axis] == file.integer<int16_t>(), "centre");
        if(file.integer<uint8_t>()) {
          auto const &mesh{std::get<earthsiege::model_mesh>(part->content)};
          require(mesh.points.size() == file.integer<uint16_t>(), "point count");
          for(auto const &point : mesh.points) {
            for(unsigned int axis{0}; axis != 3; ++axis) require(point[axis] == file.integer<int16_t>(), "point");
            ++points;
          }
        }
        if(file.integer<uint8_t>()) {
          auto const &tree{std::get<earthsiege::bsp_part>(part->content)};
          require(tree.nodes.size() == file.integer<uint16_t>(), "BSP count");
          for(auto const &entry : tree.nodes) require(entry.transform == file.integer<int16_t>(), "BSP transform");
        }
        ++nodes;
      }
      ++weapons;
    }
  }
  file.require_end();
  if(count != 8 || weapons != 32) throw std::runtime_error{"weapon assembly fixture coverage differs"};
  std::cout << weapons << " original weapon assemblies, " << nodes << " nodes and " << points << " complete mesh points agree" << std::endl;
  return EXIT_SUCCESS;
} catch(std::exception const &error) {
  std::cerr << error.what() << std::endl;
  return EXIT_FAILURE;
}
