#include <cstdlib>
#include <iostream>
#include <string>
#include "resources/input_tape.h"
#include "simulation/mission_session.h"

int main(int argc, char **argv) try {
  using namespace earthsiege;
  if(argc != 3) return EXIT_FAILURE;
  resource_store resources{argv[1]};
  auto const data{read_file(argv[2])}; binary_reader file{data};
  auto const magic{file.take(8)};
  if(std::string(magic.begin(), magic.end()) != "ESMDSP01") file.fail("display fixture version");
  auto const worlds{file.integer<uint32_t>()}; size_t checked{};
  for(uint32_t world{}; world < worlds; ++world) {
    auto const demo{file.integer<uint32_t>()}, count{file.integer<uint32_t>()}, player{file.integer<uint32_t>()};
    auto const tape{read_input_tape(read_file(resources.root() / "TAPES" / ("DEMO" + std::to_string(demo) + ".TAP")))};
    auto const script{read_mission_script(tape.starting_files[std::to_underlying(tape_file::script)])};
    auto const roster{read_player_roster(tape.starting_files[std::to_underlying(tape_file::player)])};
    auto const counters{read_mission_counters(tape.starting_files[std::to_underlying(tape_file::campaign_flags)])};
    mission_session session{resources, script, roster, random_state{}, counters};
    if(session.player_id() != player || session.activity_state().size() != count) file.fail("mission identity");
    auto const scanner{session.scanner_state(0, 0, 120000)};
    if(scanner.contacts.size() + 1 != count) file.fail("scanner roster size");
    size_t contact{};
    for(uint32_t i{}; i < count; ++i) {
      auto const prefix{"demo " + std::to_string(demo) + " actor " + std::to_string(i) + " "};
      auto const check{[&](std::string const &name, auto expected, auto actual) {
        if(expected != actual) { std::cerr << prefix << name << ": expected " << +expected << ", actual " << +actual << '\n'; file.fail(name); }
      }};
      auto const state{session.instrument_damage(i)};
      auto const kind{file.integer<int16_t>()}; check("kind", kind, std::to_underlying(state.kind));
      check("model", file.integer<int16_t>(), state.model);
      check("friendly", file.integer<uint8_t>(), state.friendly);
      check("player", file.integer<uint8_t>(), state.player);
      check("dead", file.integer<uint8_t>(), state.dead);
      check("shield warning", file.integer<uint8_t>(), state.shield_warning);
      check("total damage", file.integer<int16_t>(), state.total_damage);
      for(auto v : state.armour) check("armour", file.integer<int16_t>(), v);
      for(auto v : state.internal) check("internal", file.integer<int16_t>(), v);
      auto const side{file.integer<uint8_t>()};
      vec2<int32_t> const position{file.integer<int32_t>(), file.integer<int32_t>()};
      vec2<int32_t> const history{file.integer<int32_t>(), file.integer<int32_t>()};
      auto const active{file.integer<uint8_t>()}, seen{file.integer<uint8_t>()}, unavailable{file.integer<uint8_t>()};
      if(i != player) {
        auto const &r{scanner.contacts.at(contact++)};
        check("scanner side", side, r.side); check("active", active, r.active); check("detected", seen, r.detected);
        check("last position unavailable", unavailable, r.last_position_unavailable);
        if(r.position != position || r.last_position != history) file.fail(prefix + "scanner positions");
        auto const expected_kind{kind == 0 ? combat_actor_kind::herc : kind == 1 ? combat_actor_kind::base : combat_actor_kind::flyer};
        if(r.kind != expected_kind || r.selected) file.fail(prefix + "scanner identity");
      } else if(scanner.player_position != position) file.fail("scanner player position");
      ++checked;
    }
    if(scanner.radar_active != (file.integer<uint8_t>() != 0)) file.fail("radar state");
    if(session.instrument_damage({}).kind != damage_display_kind::none) file.fail("empty MFD selection");
  }
  file.require_end();
  std::cout << checked << " original mission damage/scanner bindings across " << worlds << " starts match\n";
  return EXIT_SUCCESS;
} catch(std::exception const &error) { std::cerr << error.what() << '\n'; return EXIT_FAILURE; }
