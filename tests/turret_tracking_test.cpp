#include <cstdlib>
#include <iostream>
#include <string>
#include "resources/resource_store.h"
#include "simulation/ground_actor.h"

int main(int const argc, char **argv) try {
  if(argc != 3) return EXIT_FAILURE;
  earthsiege::resource_store resources{argv[1]}; earthsiege::ground_catalogue catalogue{resources};
  auto const bytes{earthsiege::read_file(argv[2])}; earthsiege::binary_reader file{bytes}; auto const magic{file.take(8)};
  if(std::string(magic.begin(),magic.end()) != "ESTAIM01") file.fail("fixture version");
  unsigned int steps{0}; auto const count{file.integer<uint16_t>()};
  for(unsigned int row{0}; row != count; ++row) {
    vec3<int32_t> const position{file.integer<int32_t>(),file.integer<int32_t>(),file.integer<int32_t>()}; auto const heading{file.integer<uint16_t>()};
    earthsiege::ground_actor actor{catalogue,8,1,100,position,{0,0,heading}};
    auto const amount{file.integer<uint16_t>()};
    for(unsigned int step{0}; step != amount; ++step) {
      vec3<int32_t> const target{file.integer<int32_t>(),file.integer<int32_t>(),file.integer<int32_t>()};
      vec3<int16_t> const expected{file.integer<int16_t>(),file.integer<int16_t>(),file.integer<int16_t>()};
      auto const error{actor.track_point(target)}; auto const label{std::to_string(row) + '/' + std::to_string(step)};
      if(error != expected) file.fail(label + " target angles");
      for(auto value : actor.turret_aim().rates) if(value != file.integer<int16_t>()) file.fail(label + " rate");
      for(auto value : actor.turret_aim().angles) if(value != file.integer<int16_t>()) file.fail(label + " angle");
      if(actor.instance().transforms().size() != file.integer<uint16_t>()) file.fail(label + " pose count");
      for(auto const &matrix : actor.instance().transforms()) {
        for(unsigned int const field : {0U,1U,3U,4U,2U,5U,6U,7U,8U}) if(matrix.basis.data[field] != file.integer<int16_t>()) file.fail(label + " basis");
        if(matrix.type != file.integer<uint8_t>()) file.fail(label + " matrix type");
        file.integer<uint8_t>();
        for(unsigned int axis{0}; axis != 3; ++axis) if(matrix.translation[axis] != file.integer<int32_t>()) file.fail(label + " translation");
      }
      ++steps;
    }
  }
  file.require_end(); if(count != 64 || steps != 1536) file.fail("tracking coverage differs");
  std::cout << steps << " original turret tracking updates agree\n";
  return EXIT_SUCCESS;
} catch(std::exception const &error) { std::cerr << error.what() << '\n'; return EXIT_FAILURE; }
