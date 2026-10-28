#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include "resources/input_tape.h"
#include "simulation/mission_session.h"

int main(int argc,char **argv) try {
  using namespace earthsiege;
  if(argc!=3)return EXIT_FAILURE;
  resource_store resources{argv[1]};auto const source{read_file(argv[2])};binary_reader file{source};auto const magic{file.take(8)};
  if(std::string(magic.begin(),magic.end())!="ESMHUD01")file.fail("fixture version");
  auto const count{file.integer<uint32_t>()};unsigned int actors{};
  for(unsigned int i{};i<count;++i) {
    auto const demo{file.integer<uint32_t>()},total{file.integer<uint32_t>()};
    auto const tape{read_input_tape(read_file(resources.root()/"TAPES"/("DEMO"+std::to_string(demo)+".TAP")))};
    auto const script{read_mission_script(tape.starting_files[std::to_underlying(tape_file::script)])};
    auto const roster{read_player_roster(tape.starting_files[std::to_underlying(tape_file::player)])};
    auto const counters{read_mission_counters(tape.starting_files[std::to_underlying(tape_file::campaign_flags)])};
    mission_session session{resources,script,roster,random_state{},counters};
    if(session.activity_state().size()!=total || session.target_marker())file.fail("initial HUD state");
    for(unsigned int actor{};actor<total;++actor) {
      vec3<int32_t> const position{file.integer<int32_t>(),file.integer<int32_t>(),file.integer<int32_t>()};
      auto const radius{file.integer<int16_t>()};auto const side{file.integer<uint8_t>()};
      session.set_displayed_target(actor);auto const marker{session.target_marker()};
      if(!marker || marker->position!=position || marker->model_radius!=radius || marker->side!=side)
        file.fail("mission "+std::to_string(demo)+" actor "+std::to_string(actor)+" HUD input differs");
      ++actors;
    }
    auto const waypoint{session.next_waypoint()};bool const present{file.integer<uint8_t>()!=0};
    if(waypoint.has_value()!=present)file.fail("next waypoint presence");
    if(present) {
      auto const number{file.integer<int16_t>()};vec3<int32_t> const point{file.integer<int32_t>(),file.integer<int32_t>(),file.integer<int32_t>()};
      if(waypoint->number!=number || waypoint->position!=point)file.fail("next waypoint state");
    }
    session.set_displayed_target({});if(session.target_marker())file.fail("target clear");
  }
  file.require_end();std::cout<<count<<" original mission HUD starts and "<<actors<<" actor inputs match\n";return EXIT_SUCCESS;
} catch(std::exception const &e) {std::cerr<<e.what()<<'\n';return EXIT_FAILURE;}
