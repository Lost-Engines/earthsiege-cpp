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
std::vector<int16_t> words(earthsiege::binary_reader &file) {
  std::vector<int16_t> r(file.integer<uint16_t>());for(auto &v:r)v=file.integer<int16_t>();return r;
}
}
int main(int argc,char **argv) try {
  using namespace earthsiege;if(argc!=3)return EXIT_FAILURE;
  resource_store resources{argv[1]};auto const bytes{read_file(argv[2])};binary_reader file{bytes};auto const magic{file.take(8)};
  if(std::string(magic.begin(),magic.end())!="ESWFRM01")file.fail("fixture version");
  auto const count{file.integer<uint16_t>()};size_t frames{0},states{0};
  for(size_t trial{0};trial!=count;++trial) {
    auto const demo{file.integer<uint16_t>()};auto const ratio{file.integer<int16_t>()};auto const initial{random_input(file)};
    auto const tape{read_input_tape(read_file(resources.root()/"TAPES"/("DEMO"+std::to_string(demo)+".TAP")))};
    auto script{read_mission_script(tape.starting_files[std::to_underlying(tape_file::script)])};
    for(auto &g:script.groups) {g.mode=0;g.trigger=-1;}
    auto const roster{read_player_roster(tape.starting_files[std::to_underlying(tape_file::player)])};
    auto const counters{read_mission_counters(tape.starting_files[std::to_underlying(tape_file::campaign_flags)])};
    mission_session session{resources,script,roster,random_state{initial},counters};
    auto const steps{file.integer<uint16_t>()},nh{file.integer<uint16_t>()},nf{file.integer<uint16_t>()};
    for(size_t step{0};step!=steps;++step) {
      session.step({},ratio);auto const hercs{session.hercs()};auto const flyers{session.flying_actors()};
      if(hercs.size()!=nh || flyers.size()!=nf)file.fail("actor count");
      auto const prefix{std::to_string(demo)+"/"+std::to_string(ratio)+" frame "+std::to_string(step)};
      for(size_t i{0};i!=nh;++i) {
        auto const label{prefix+" HERC "+std::to_string(i)};
        auto const &actor{hercs[i].actor};auto const &body{actor.body()};auto const &motion{body.movement()};
        auto const &decision{session.activity_state().decisions()[i]};auto const &controller{decision.controller};
        auto const check{[&](std::string const &field,auto expected,auto actual){
          if(expected!=actual) {std::cerr<<label<<' '<<field<<": expected "<<expected<<", actual "<<actual<<'\n';file.fail(label+' '+field);}
        }};
        for(unsigned axis{0};axis!=3;++axis)check("position["+std::to_string(axis)+"]",file.integer<int32_t>(),motion.position[axis]);
        for(unsigned axis{0};axis!=3;++axis)check("angles["+std::to_string(axis)+"]",file.integer<uint16_t>(),motion.angles[axis]);
        check("steering",file.integer<int16_t>(),motion.steering_rate);check("gait",file.integer<int16_t>(),motion.gait_rate);
        check("throttle",file.integer<int16_t>(),motion.throttle);check("playback",file.integer<int16_t>(),motion.playback_rate);
        check("torso rate",file.integer<int16_t>(),body.torso().rate);check("elevation rate",file.integer<int16_t>(),body.elevation().rate);
        check("torso",file.integer<int16_t>(),body.torso().angle);check("elevation",file.integer<int16_t>(),body.elevation().angle);
        check("controller",file.integer<uint16_t>(),controller.type?std::to_underlying(*controller.type):-1);
        check("clock",file.integer<int32_t>(),controller.reconsider_remaining);check("ticks",file.integer<uint32_t>(),controller.ticks);
        auto const timer{file.integer<int32_t>()};
        if(controller.type==behavior::patrolling || controller.type==behavior::search_destroy || controller.type==behavior::guarding)
          check("search timer",timer,controller.target_search_remaining);
        check("route",file.integer<int16_t>(),session.activity_state().group(hercs[i].group).route_cursor);
        check("recovery",file.integer<int32_t>(),motion.collision_recovery);file.integer<int16_t>();
        check("reverse",file.integer<uint8_t>()!=0,motion.reverse_recovery);
        auto const &damage{actor.damage()};auto const &c{damage.consequences};
        bool const flags[]{actor.impacts().detectable,c.dead,c.disabled,decision.unavailable,c.movement_light,c.movement_heavy,c.reactor_light,c.reactor_heavy,motion.collapsed};
        for(size_t j{0};j!=9;++j)check("flag "+std::to_string(j),file.integer<uint8_t>()!=0,flags[j]);
        check("power",file.integer<int16_t>(),actor.power().reserve);auto const &shields{actor.power().shields};
        check("front shield",file.integer<int16_t>(),shields.front);check("rear shield",file.integer<int16_t>(),shields.rear);
        check("shield balance",file.integer<int16_t>(),shields.balance);check("shield capacity",file.integer<int16_t>(),shields.capacity);
        check("strength",file.integer<int16_t>(),actor.combat_rating());
        for(auto const &[name,actual]:{std::pair{"internal",std::span<int16_t const>{damage.internal}}, {"armour",std::span<int16_t const>{damage.armour}}, {"present",std::span<int16_t const>{damage.present}}}) {
          auto const expected{words(file)};check(std::string(name)+" size",expected.size(),actual.size());
          for(size_t j{0};j!=expected.size();++j)check(std::string(name)+'['+std::to_string(j)+']',expected[j],actual[j]);
        }
        auto const expected_cells{words(file)};auto const actual_cells{body.model().cell_frames()};
        check("cell count",expected_cells.size(),actual_cells.size());
        for(size_t j{0};j!=expected_cells.size();++j)check("cell "+std::to_string(j),static_cast<uint16_t>(expected_cells[j]),actual_cells[j]);
        ++states;
      }
      for(size_t i{0};i!=nf;++i) {
        vec3<int32_t> const position{file.integer<int32_t>(),file.integer<int32_t>(),file.integer<int32_t>()};
        vec3<uint16_t> const angles{file.integer<uint16_t>(),file.integer<uint16_t>(),file.integer<uint16_t>()};
        vec3<int16_t> const velocity{file.integer<int16_t>(),file.integer<int16_t>(),file.integer<int16_t>()};
        vec3<int16_t> const rates{file.integer<int16_t>(),file.integer<int16_t>(),file.integer<int16_t>()};
        auto const strength{file.integer<int16_t>()},timer{file.integer<int16_t>()};bool const obstacle{file.integer<uint8_t>()!=0};
        auto const type{file.integer<uint16_t>()};auto const clock{file.integer<int32_t>()};bool const detectable{file.integer<uint8_t>()!=0};
        auto const label{prefix+" flyer "+std::to_string(i)};
        auto const &flight{flyers[i].actor.motion()};auto const &controller{session.activity_state().decisions()[nh+i].controller};
        if(position!=flight.position || angles!=flight.angles || velocity!=flight.velocity || rates!=flight.angular_step)file.fail(label+" motion");
        if(strength!=flight.obstacle_strength || timer!=flight.obstacle_timer || obstacle!=flight.obstacle || detectable!=flyers[i].actor.is_detectable())file.fail(label+" avoidance/sensor");
        if(!controller.type || std::to_underlying(*controller.type)!=type || controller.reconsider_remaining!=clock)file.fail(label+" controller");
        ++states;
      }
      auto const expected{random_input(file)};auto const &actual{session.random_snapshot()};
      if(actual.words!=expected.words || actual.first!=expected.first || actual.second!=expected.second)file.fail(prefix+" RNG");
      ++frames;
    }
  }
  file.require_end();if(frames!=960)file.fail("frame coverage");
  std::cout<<frames<<" original world frames match, including "<<states<<" HERC/flyer states\n";return EXIT_SUCCESS;
} catch(std::exception const &e){std::cerr<<e.what()<<'\n';return EXIT_FAILURE;}
