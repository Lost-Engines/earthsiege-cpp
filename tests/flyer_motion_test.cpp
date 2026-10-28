#include <array>
#include <cstdlib>
#include <iostream>
#include <string>
#include "resources/resource_store.h"
#include "simulation/flyer_motion.h"

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
  if(std::string(magic.begin(),magic.end()) != "ESFMOVE1") file.fail("fixture version");
  unsigned int steps{0}, probes{0}; auto const paths{file.integer<uint16_t>()};
  for(unsigned int path{0}; path != paths; ++path) {
    auto state{read_state(file)}; auto const count{file.integer<uint16_t>()};
    for(unsigned int step{0}; step != count; ++step) {
      auto const op{file.integer<uint8_t>()}; std::array<int32_t,4> args; for(auto &v : args) v = file.integer<int32_t>(); auto const steered{read_state(file)};
      switch(op) {
        case 0: aim_flyer_pitch(state,definition.profile,static_cast<int16_t>(args[0])); break;
        case 1: steer_flyer(state,definition.profile,static_cast<int16_t>(args[0]),static_cast<int16_t>(args[1])); break;
        case 2: hold_flyer_bank(state,definition.profile,static_cast<int16_t>(args[0])); break;
        case 3: turn_flyer(state,definition.profile,static_cast<int16_t>(args[0]),args[1] ? std::optional<flyer_leader_turn>{{static_cast<uint16_t>(args[2]),static_cast<int16_t>(args[3])}} : std::nullopt); break;
        case 4: hold_flyer_altitude(state,definition.profile,args[0]); break;
        default: file.fail("steering operation");
      }
      auto const label{std::to_string(path) + '/' + std::to_string(step) + " op " + std::to_string(op)};
      compare(state,steered,label + " steering");
      auto const ratio{file.integer<int16_t>()}; std::array<std::optional<int32_t>,2> distances;
      for(auto &d : distances) { bool const present{file.integer<uint8_t>() != 0}; auto const value{file.integer<int32_t>()}; if(present) d = value; }
      std::vector<std::array<int32_t,7>> expected_probes(file.integer<uint8_t>());
      for(auto &q : expected_probes) { q[0] = file.integer<uint8_t>(); for(size_t i{1}; i != q.size(); ++i) q[i] = file.integer<int32_t>(); }
      auto const expected{read_state(file)}; std::vector<std::array<int32_t,7>> actual;
      advance_flyer(state,definition.profile,ratio,[&](avoidance_probe kind,vec3<int32_t> const &start,vec3<int32_t> const &end){
        auto const index{kind == avoidance_probe::scenery ? 0U : 1U}; actual.push_back({static_cast<int32_t>(index),start.x,start.y,start.z,end.x,end.y,end.z}); return distances[index];
      });
      if(actual != expected_probes) file.fail(label + " obstacle probes");
      compare(state,expected,label + " movement"); probes += static_cast<unsigned int>(actual.size()); ++steps;
    }
  }
  file.require_end(); if(paths != 192 || steps != 2304 || probes != 4608) file.fail("flyer motion coverage");
  std::cout << steps << " original flyer steering/movement steps and " << probes << " obstacle queries agree\n"; return EXIT_SUCCESS;
} catch(std::exception const &error) { std::cerr << error.what() << '\n'; return EXIT_FAILURE; }
