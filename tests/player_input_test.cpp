#include <array>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>
#include "resources/binary_reader.h"
#include "simulation/player_input.h"

int main(int const argc, char **argv) try {
  if(argc != 2) return EXIT_FAILURE;
  auto const data{earthsiege::read_file(argv[1])}; earthsiege::binary_reader file{data};
  auto const magic{file.take(8)}; if(std::string(magic.begin(), magic.end()) != "ESPINP01") file.fail("fixture version");
  auto const count{file.integer<uint32_t>()}; earthsiege::player_input_router router;
  auto const integer{[&]{return file.integer<int32_t>();}};
  auto const word{[&]{return static_cast<int16_t>(integer());}};
  auto const boolean{[&]{return integer() != 0;}};
  for(uint32_t row{0}; row != count; ++row) {
    earthsiege::player_input_context context;
    context.external_control = boolean(); context.controlled_flyer = boolean(); context.active = boolean(); context.disabled = boolean(); context.auto_tracking = boolean();
    bool const target{boolean()}, dead{boolean()};
    earthsiege::player_input input;
    input.movement = {word(), word()}; input.aiming = {word(), word()}; input.camera = {word(), word()}; input.mouse = {integer(), integer()};
    input.left = boolean(); input.right = boolean(); input.fire = boolean(); input.enter = boolean(); input.released = boolean();
    context.release_action = static_cast<earthsiege::control_release_action>(integer());
    context.position = {integer(), integer(), integer()};
    earthsiege::player_input_target const candidate{{integer(), integer(), integer()}, {word(), word(), word()}, dead};
    if(target) context.target = candidate;
    if(boolean()) router.request_centre();
    auto const command{integer()}, result{integer()};
    if((command == 2) != (result != 0)) file.fail("original exit command result");
    auto const output{router.route(input, context)};
    std::vector<std::vector<int32_t>> expected, actual;
    auto const events{file.integer<uint16_t>()};
    for(unsigned int i{0}; i != events; ++i) {
      std::vector<int32_t> event{file.integer<uint16_t>()}; auto const fields{file.integer<uint16_t>()};
      for(unsigned int j{0}; j != fields; ++j) event.push_back(integer());
      expected.push_back(event);
    }
    actual.push_back({0, output.zoom, output.camera.y, output.camera.x});
    if(output.movement) actual.push_back({output.move_flyer ? 2 : 1, output.movement->y, output.movement->x});
    switch(output.aim) {
      case earthsiege::player_aim_mode::none: break;
      case earthsiege::player_aim_mode::manual: actual.push_back({3, output.aiming.x}); actual.push_back({4, output.distance, output.aiming.y}); break;
      case earthsiege::player_aim_mode::centre: actual.push_back({6, output.distance}); break;
      case earthsiege::player_aim_mode::target: actual.push_back({5, output.target_point.x, output.target_point.y, output.target_point.z}); break;
    }
    if(output.fire_allowed) actual.push_back({7, output.fire});
    if(output.cycle_target) actual.push_back({8});
    if(actual != expected) {
      std::cerr << "actual:"; for(auto const &event : actual) { std::cerr << " ["; for(auto v : event) std::cerr << v << ','; std::cerr << ']'; }
      std::cerr << "\nexpected:"; for(auto const &event : expected) { std::cerr << " ["; for(auto v : event) std::cerr << v << ','; std::cerr << ']'; } std::cerr << '\n';
      file.fail("input routing differs at " + std::to_string(row));
    }
    auto const origin{router.origin()}, response{router.response()};
    std::array<int32_t, 11> const state{router.aiming_with_mouse(), origin.x, origin.y, response.x, response.y, router.centred(), router.enter_pressed(), output.reset_aim, output.stop, output.stop, output.move_flyer};
    for(size_t field{0}; field != state.size(); ++field) {
      auto const value{integer()}; if(state[field] != value) file.fail("input state " + std::to_string(row) + '/' + std::to_string(field) + ": " + std::to_string(state[field]) + " != " + std::to_string(value));
    }
  }
  file.require_end(); if(count != 4096) file.fail("player input coverage differs");
  std::cout << count << " original player control-routing updates agree\n"; return EXIT_SUCCESS;
} catch(std::exception const &e) { std::cerr << e.what() << '\n'; return EXIT_FAILURE; }
