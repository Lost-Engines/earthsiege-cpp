#include <array>
#include <cstdlib>
#include <iostream>
#include <string>
#include "resources/resource_store.h"
#include "simulation/flyer_navigation.h"

namespace {
using namespace earthsiege;
flyer_motion read_state(binary_reader &file) {
  return {
    {file.integer<int32_t>(),file.integer<int32_t>(),file.integer<int32_t>()},
    {file.integer<uint16_t>(),file.integer<uint16_t>(),file.integer<uint16_t>()},
    {file.integer<int16_t>(),file.integer<int16_t>(),file.integer<int16_t>()},
    {file.integer<int16_t>(),file.integer<int16_t>(),file.integer<int16_t>()},
    file.integer<int16_t>(),file.integer<int16_t>(),file.integer<uint8_t>() != 0,
  };
}
void compare(flyer_motion const &a, flyer_motion const &e, std::string const &label) {
  if(a.position != e.position || a.angles != e.angles || a.velocity != e.velocity || a.angular_step != e.angular_step || a.obstacle_strength != e.obstacle_strength || a.obstacle_timer != e.obstacle_timer || a.obstacle != e.obstacle) {
    std::cerr << label << " actual position " << a.position << " angles " << a.angles << " rates " << a.angular_step << " strength/timer " << a.obstacle_strength << '/' << a.obstacle_timer << '\n';
    std::cerr << label << " expected position " << e.position << " angles " << e.angles << " rates " << e.angular_step << " strength/timer " << e.obstacle_strength << '/' << e.obstacle_timer << '\n';
    throw std::runtime_error{label + " flight state"};
  }
}
}
int main(int const argc, char **argv) try {
  if(argc != 3) return EXIT_FAILURE;
  resource_store resources{argv[1]}; auto const definition{read_flyer_definition(resources,"SKIMMER")};
  auto const bytes{read_file(argv[2])}; binary_reader file{bytes}; auto const magic{file.take(8)};
  if(std::string(magic.begin(),magic.end()) != "ESFNAV01") file.fail("fixture version");
  auto const count{file.integer<uint16_t>()}; unsigned int routes{0}, formations{0};
  for(unsigned int row{0}; row != count; ++row) {
    bool const follower{file.integer<uint8_t>() != 0}; auto state{read_state(file)}; auto speed{file.integer<int16_t>()};
    if(!follower) {
      std::vector<vec3<int32_t>> points(file.integer<uint16_t>()); mission_route route;
      for(auto &p : points) { p = vec3<int32_t>{file.integer<int32_t>(),file.integer<int32_t>(),file.integer<int32_t>()}; route.points.push_back(static_cast<int16_t>(route.points.size())); }
      bool const closed{file.integer<uint8_t>() != 0}; if(closed) route.points.back() = 0;
      auto cursor{file.integer<int16_t>()}; auto const expected_cursor{file.integer<int16_t>()};
      follow_flyer_route(state,definition.profile,route,points,cursor,speed);
      if(cursor != expected_cursor) file.fail("flyer route cursor");
      ++routes;
    } else {
      auto const leader{read_state(file)}; vec3<int32_t> const offset{file.integer<int32_t>(),file.integer<int32_t>(),file.integer<int32_t>()};
      follow_flyer_formation(state,definition.profile,leader,offset,speed); ++formations;
    }
    auto const expected{read_state(file)}; auto const expected_speed{file.integer<int16_t>()};
    compare(state,expected,std::to_string(row));
    if(speed != expected_speed) file.fail(std::to_string(row) + " requested speed");
  }
  file.require_end(); if(count != 1200 || routes != 600 || formations != 600) file.fail("flyer navigation coverage");
  std::cout << routes << " route and " << formations << " formation updates match the original\n"; return EXIT_SUCCESS;
} catch(std::exception const &error) { std::cerr << error.what() << '\n'; return EXIT_FAILURE; }
