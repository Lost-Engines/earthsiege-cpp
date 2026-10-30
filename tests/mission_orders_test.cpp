#include "resources/input_tape.h"
#include "simulation/mission_session.h"
#include <cstdlib>
#include <iostream>
#include <string>

int main(int argc,char **argv) try {
  using namespace earthsiege;
  if(argc!=3) return EXIT_FAILURE;
  resource_store resources{argv[1]};auto const bytes{read_file(argv[2])};binary_reader file{bytes};auto const magic{file.take(8)};
  if(std::string(magic.begin(),magic.end())!="ESMORD01") file.fail("fixture version");
  auto const read{[&]{return file.integer<int32_t>();}};
  auto const ref{[&]()->std::optional<size_t>{auto const value{read()};return value<0?std::nullopt:std::optional<size_t>{static_cast<size_t>(value)};}};
  auto const worlds{file.integer<uint32_t>()};size_t total{};
  for(uint32_t w{};w<worlds;++w) {
    auto const demo{read()},actors{read()},steps{read()};
    auto const tape{read_input_tape(read_file(resources.root()/"TAPES"/("DEMO"+std::to_string(demo)+".TAP")))};
    mission_session session{resources,read_mission_script(tape.starting_files[std::to_underlying(tape_file::script)]),read_player_roster(tape.starting_files[std::to_underlying(tape_file::player)]),random_state{},read_mission_counters(tape.starting_files[std::to_underlying(tape_file::campaign_flags)])};
    for(int step{};step<steps;++step) {
      auto const command{static_cast<uint16_t>(read())};session.set_displayed_target(ref());
      auto const label{"demo "+std::to_string(demo)+" step "+std::to_string(step)+" command "+std::to_string(command)};
      auto const check{[&](std::string const &field,int expected,int actual){if(expected!=actual){std::cerr<<label<<' '<<field<<": expected "<<expected<<", actual "<<actual<<'\n';file.fail(field);}}};
      bool const accepted{session.transmit_order(command)};check("accepted",read(),accepted);
      auto const replies{session.take_radio_events()};check("reply count",read(),static_cast<int>(replies.size()));
      for(auto const &r:replies){check("speaker",read(),static_cast<int>(r.speaker.value()));check("reply",read(),r.id);}
      for(int i{};i<actors;++i) {
        auto const s{session.order_state(static_cast<size_t>(i))};auto const &decision{session.activity_state().decisions()[static_cast<size_t>(i)]};
        auto const prefix{"actor "+std::to_string(i)+" "};
        check(prefix+"command",read(),s.command);check(prefix+"designated",read(),s.designated?static_cast<int>(*s.designated):-1);check(prefix+"stop status",read(),s.stop_status);
        check(prefix+"target",read(),s.selected?static_cast<int>(*s.selected):-1);check(prefix+"behavior",read(),std::to_underlying(s.type));check(prefix+"clock",read(),s.reconsider_remaining);
        check(prefix+"radar",read(),s.radar);check(prefix+"radar request",read(),s.radar_requested);check(prefix+"ignore",read(),s.ignore_target);check(prefix+"damage",read(),s.incoming_damage);
        check(prefix+"attack requested",read(),decision.attack_requested);auto const component{read()}; if(read()!=0) check(prefix+"aimed component",component,decision.aimed_component);
      }
      check("stagger",read(),session.activity_state().entry_stagger());
      auto const &random{session.random_snapshot()};
      for(auto value:random.words)check("random word",static_cast<uint16_t>(read()),value);
      check("random first",read(),static_cast<int>(random.first));check("random second",read(),static_cast<int>(random.second));++total;
    }
  }
  file.require_end();std::cout<<total<<" original live mission command transitions match\n";return EXIT_SUCCESS;
} catch(std::exception const &e){std::cerr<<e.what()<<'\n';return EXIT_FAILURE;}
