#include <cstdlib>
#include <iostream>
#include <string>
#include "resources/input_tape.h"
#include "resources/resource_store.h"
#include "simulation/mission_session.h"
namespace {
earthsiege::random_state::state random_input(earthsiege::binary_reader &file) {
  earthsiege::random_state::state r;for(auto &x:r.words)x=file.integer<uint16_t>();r.first=file.integer<uint8_t>();r.second=file.integer<uint8_t>();return r;
}
}
int main(int argc,char **argv) try {
  using namespace earthsiege;if(argc!=3)return EXIT_FAILURE;
  resource_store resources{argv[1]};auto const bytes{read_file(argv[2])};binary_reader file{bytes};auto const magic{file.take(8)};
  if(std::string(magic.begin(),magic.end())!="ESFFRM01")file.fail("fixture version");
  auto const count{file.integer<uint16_t>()};size_t frames{0},states{0};
  for(size_t trial{0};trial!=count;++trial) {
    auto const demo{file.integer<uint16_t>()};auto const ratio{file.integer<int16_t>()};auto const initial{random_input(file)};
    auto const tape{read_input_tape(read_file(resources.root()/"TAPES"/("DEMO"+std::to_string(demo)+".TAP")))};
    auto script{read_mission_script(tape.starting_files[std::to_underlying(tape_file::script)])};
    for(auto &g:script.groups) {g.mode=0;g.trigger=-1;}
    auto const roster{read_player_roster(tape.starting_files[std::to_underlying(tape_file::player)])};
    auto const counters{read_mission_counters(tape.starting_files[std::to_underlying(tape_file::campaign_flags)])};
    mission_session session{resources,script,roster,random_state{initial},counters};
    auto const steps{file.integer<uint16_t>()},nf{file.integer<uint16_t>()};
    for(size_t step{0};step!=steps;++step) {
      session.step({},ratio);auto const flyers{session.flying_actors()};if(flyers.size()!=nf)file.fail("flyer count");
      for(size_t i{0};i!=nf;++i) {
        vec3<int32_t> const position{file.integer<int32_t>(),file.integer<int32_t>(),file.integer<int32_t>()};
        vec3<uint16_t> const angles{file.integer<uint16_t>(),file.integer<uint16_t>(),file.integer<uint16_t>()};
        vec3<int16_t> const velocity{file.integer<int16_t>(),file.integer<int16_t>(),file.integer<int16_t>()};
        vec3<int16_t> const rates{file.integer<int16_t>(),file.integer<int16_t>(),file.integer<int16_t>()};
        auto const strength{file.integer<int16_t>()},timer{file.integer<int16_t>()};bool const obstacle{file.integer<uint8_t>()!=0};
        auto const type{file.integer<uint16_t>()};auto const clock{file.integer<int32_t>()};bool const detectable{file.integer<uint8_t>()!=0};
        auto const label{std::to_string(demo)+"/"+std::to_string(ratio)+" frame "+std::to_string(step)+" flyer "+std::to_string(i)};
        auto const &flight{flyers[i].actor.motion()};auto const &controller{session.activity_state().decisions()[session.hercs().size()+i].controller};
        if(position!=flight.position || angles!=flight.angles || velocity!=flight.velocity || rates!=flight.angular_step) {
          auto const dump{[](auto const &v){std::cerr<<v.x<<","<<v.y<<","<<v.z<<" ";}};
          dump(position);dump(flight.position);dump(angles);dump(flight.angles);dump(velocity);dump(flight.velocity);dump(rates);dump(flight.angular_step);std::cerr<<'\n';file.fail(label+" motion");
        }
        if(strength!=flight.obstacle_strength || timer!=flight.obstacle_timer || obstacle!=flight.obstacle || detectable!=flyers[i].actor.is_detectable())file.fail(label+" avoidance/sensor");
        if(!controller.type || std::to_underlying(*controller.type)!=type || controller.reconsider_remaining!=clock)file.fail(label+" controller");
        ++states;
      }
      auto const expected{random_input(file)};auto const &actual{session.random_snapshot()};
      if(actual.words!=expected.words || actual.first!=expected.first || actual.second!=expected.second)file.fail(std::to_string(demo)+"/"+std::to_string(ratio)+" frame "+std::to_string(step)+" RNG");
      ++frames;
    }
  }
  file.require_end();if(frames!=960)file.fail("frame coverage");
  std::cout<<frames<<" original world frames match, including "<<states<<" complete flyer states\n";return EXIT_SUCCESS;
} catch(std::exception const &e){std::cerr<<e.what()<<'\n';return EXIT_FAILURE;}
