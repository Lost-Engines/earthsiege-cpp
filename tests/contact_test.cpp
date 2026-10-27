#include <array>
#include <cstdlib>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>
#include "resources/resource_store.h"
#include "simulation/actor_contact.h"
#include "simulation/model_math.h"
#include "simulation/scenery_collision.h"

namespace {

vec3<int32_t> point(earthsiege::binary_reader &file) {
  return vec3<int32_t>{file.integer<int32_t>(), file.integer<int32_t>(), file.integer<int32_t>()};
}

void require(bool const okay, std::string const &label) {
  if(!okay) throw std::runtime_error{label + " differs"};
}

} // namespace

int main(int const argc, char **argv) try {
  if(argc != 4) return EXIT_FAILURE;
  earthsiege::resource_store resources{argv[1]};
  auto const actor_bytes{earthsiege::read_file(argv[2])};
  earthsiege::binary_reader actor_file{actor_bytes};
  auto const magic{actor_file.take(8)};
  if(std::string(magic.begin(), magic.end()) != "ESCONT01") actor_file.fail("fixture version");
  auto const actor_cases{actor_file.integer<uint16_t>()};
  unsigned int impacts{0};
  for(unsigned int row{0}; row != actor_cases; ++row) {
    std::array<earthsiege::contact_actor, 2> actors;
    for(auto &actor : actors) {
      actor.position = point(actor_file);
      actor.angles = vec3<uint16_t>{actor_file.integer<uint16_t>(), actor_file.integer<uint16_t>(), actor_file.integer<uint16_t>()};
      actor.radius = actor_file.integer<int16_t>();
      actor.gait_rate = actor_file.integer<int16_t>();
      actor.mass_factor = actor_file.integer<int16_t>();
      actor.impact_height = actor_file.integer<int32_t>();
      actor.herc = actor_file.integer<uint8_t>() != 0;
      actor.waiting = actor_file.integer<uint8_t>() != 0;
    }
    bool const expected_hit{actor_file.integer<uint8_t>() != 0};
    auto const expected_damage{actor_file.integer<int32_t>()};
    auto const expected_point{point(actor_file)};
    auto const actual{earthsiege::find_actor_contact(actors, 0)};
    auto const label{"actor contact " + std::to_string(row)};
    require(actual.has_value() == expected_hit, label);
    if(actual) {
      require(actual->other == 1 && actual->damage == expected_damage, label + " damage");
      require(actual->position == expected_point, label + " point");
      if(actual->damage) ++impacts;
    }
  }
  actor_file.require_end();
  auto const scenery_bytes{earthsiege::read_file(argv[3])};
  earthsiege::binary_reader scenery_file{scenery_bytes};
  auto const scenery_magic{scenery_file.take(8)};
  if(std::string(scenery_magic.begin(), scenery_magic.end()) != "ESOCCU01") scenery_file.fail("fixture version");
  auto const fields{scenery_file.integer<uint16_t>()};
  std::map<std::string, earthsiege::model_resource> models;
  unsigned int scenery_cases{0};
  for(unsigned int row{0}; row != fields; ++row) {
    auto const name{scenery_file.cstring()};
    auto const root{scenery_file.integer<uint16_t>()};
    auto const split{name.find('/')};
    if(!models.contains(name)) models.emplace(name, earthsiege::read_model(resources.read(name.substr(0, split), name.substr(split + 1))));
    auto const &model{models.at(name).roots.at(root)};
    auto const &field{*std::get<earthsiege::shape_part>(model.content).collision};
    auto const cases{scenery_file.integer<uint16_t>()};
    for(unsigned int sample{0}; sample != cases; ++sample) {
      std::vector<earthsiege::scenery_collider> objects;
      auto const count{scenery_file.integer<uint16_t>()};
      for(unsigned int object{0}; object != count; ++object) {
        auto const position{point(scenery_file)};
        auto transform{earthsiege::rotation_transform(vec3<uint16_t>{0, 0, scenery_file.integer<uint16_t>()})};
        transform.translation = position;
        objects.push_back({.field{&field}, .placement{transform}, .radius{model.header.radius}});
      }
      auto const position{point(scenery_file)};
      scenery_file.integer<int16_t>();                                          // body radius is unused by the original occupancy consumer
      auto const expected{scenery_file.integer<int32_t>()};
      require(earthsiege::scenery_contact_height(objects, position) == expected, name + '/' + std::to_string(root) + " contact " + std::to_string(sample));
      ++scenery_cases;
    }
  }
  scenery_file.require_end();
  std::cout << actor_cases << " actor contacts, " << impacts << " ramming impacts and " << scenery_cases << " scenery contacts agree" << std::endl;
  return EXIT_SUCCESS;
} catch(std::exception const &error) {
  std::cerr << error.what() << std::endl;
  return EXIT_FAILURE;
}
