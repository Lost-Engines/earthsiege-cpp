#include <array>
#include <cstdlib>
#include <iostream>
#include <vector>
#include "resources/binary_reader.h"
#include "simulation/herc_orders.h"
#include "simulation/fixed_math.h"
namespace {
using namespace earthsiege;
random_state::state read_random(binary_reader &file) {
  random_state::state state; for(auto &v : state.words) v = file.integer<uint16_t>();
  state.first = file.integer<uint8_t>(); state.second = file.integer<uint8_t>(); return state;
}
std::optional<size_t> target(bool value) { return value ? std::make_optional<size_t>(1) : std::nullopt; }
}
int main(int argc, char **argv) try {
  if(argc != 2) return EXIT_FAILURE;
  auto const bytes{read_file(argv[1])}; binary_reader file{bytes}; auto const magic{file.take(8)};
  if(std::string(magic.begin(), magic.end()) != "ESHAU001") file.fail("fixture version");
  auto const count{file.integer<uint32_t>()}; std::array<size_t, 4> coverage{};
  for(unsigned int trial{0}; trial != count; ++trial) {
    auto const kind{file.integer<uint8_t>()}; ++coverage.at(kind);
    herc_combat_position own, threat;
    own.position = {file.integer<int32_t>(), file.integer<int32_t>(), file.integer<int32_t>()};
    threat.position = {file.integer<int32_t>(), file.integer<int32_t>(), file.integer<int32_t>()};
    own.heading = file.integer<uint16_t>(); threat.heading = file.integer<uint16_t>(); own.view_offset = file.integer<int16_t>(); threat.view_offset = file.integer<int16_t>();
    auto const ratio{file.integer<int16_t>()}; auto const timer{file.integer<int32_t>()};
    std::array<bool, 6> flags; for(auto &v : flags) v = file.integer<uint8_t>() != 0;
    std::array<target_relationship, 2> targets; targets[0].selected = target(file.integer<uint8_t>() != 0); targets[1].attackers = 10;
    auto const chosen{target(file.integer<uint8_t>() != 0)}; auto command{file.integer<int16_t>()};
    bool detectable{file.integer<uint8_t>() != 0}; bool const persistent{file.integer<uint8_t>() != 0};
    herc_travel_state travel{timer, target(file.integer<uint8_t>() != 0)};
    herc_flee_state flee{timer, file.integer<uint8_t>() != 0, 1};
    herc_skirt_state skirt{static_cast<behavior>(file.integer<uint8_t>()), file.integer<uint8_t>() != 0, file.integer<uint8_t>() != 0, timer};
    vec3<int32_t> const obstacle{file.integer<int32_t>(), file.integer<int32_t>(), file.integer<int32_t>()};
    skirt.distance = file.integer<int32_t>(); bool const blocked{file.integer<uint8_t>() != 0}; auto stagger{file.integer<uint16_t>()}; bool obstruction{true};
    herc_ram_state ram{timer, file.integer<uint8_t>() != 0, file.integer<int16_t>()};
    vec3<int32_t> const delta{file.integer<int32_t>(), file.integer<int32_t>(), file.integer<int32_t>()}; auto const turn{file.integer<int16_t>()}; random_state random{read_random(file)};
    auto const expected_result{file.integer<uint16_t>()}; std::vector<std::array<int32_t, 4>> expected(file.integer<uint8_t>()), actual;
    for(auto &e : expected) for(auto &v : e) v = file.integer<int32_t>();
    auto const final_selected{target(file.integer<uint8_t>() != 0)}; auto const final_count{file.integer<int16_t>()}; bool const changed{file.integer<uint8_t>() != 0}; auto const final_command{file.integer<int16_t>()}; bool const final_detectable{file.integer<uint8_t>() != 0};
    vec3<int32_t> const final_position{file.integer<int32_t>(), file.integer<int32_t>(), file.integer<int32_t>()}; auto const final_heading{file.integer<uint16_t>()}; auto const final_timer{file.integer<int32_t>()}; auto const final_rng{read_random(file)};
    herc_order_callbacks const callbacks{
      .position{[&](size_t actor){ return actor == 0 ? own : threat; }},
      .select{[&](target_selection_flags selection_flags){ actual.push_back({4, static_cast<int32_t>(selection_flags), 0, 0}); return chosen; }},
      .follow_route{[&]{actual.push_back({3, 0, 0, 0});}},
      .move{[&](movement_request const &r){actual.push_back({0, r.flags, r.speed, r.turn});}},
      .aim{[&](size_t actor, int16_t facing){if(actor != 1) file.fail("unexpected target"); actual.push_back({1, 0, facing, 0});}},
      .centre_aim{[&]{actual.push_back({2, 0, 0, 0});}},
      .path_blocked{[&]{actual.push_back({5, 0, 0, 0}); return blocked;}},
      .enter{[&](behavior type){actual.push_back({6, static_cast<int32_t>(type), 0, 0}); controller_state state; enter_behavior(state, type, stagger);}},
      .ram_advance{[&]{actual.push_back({7, 0, 0, 0}); own.position += delta; own.heading = static_cast<uint16_t>(own.heading + turn);}},
    };
    bool result{false};
    if(kind == 0) travel_with_herc(travel, targets, 0, persistent, detectable, ratio, callbacks);
    else if(kind == 1) result = flee_with_herc(flee, targets, 0, command, flags[0] || flags[1] || flags[2], flags[3] || flags[4] || flags[5], ratio, random, callbacks);
    else if(kind == 2) skirt_with_herc(skirt, 0, obstacle, obstruction, ratio, callbacks);
    else ram_with_herc(ram, targets, 0, ratio, random, callbacks);
    auto const label{std::to_string(trial)};
    if(actual != expected) {
      for(auto const &e : actual) std::cerr << "actual " << e[0] << ',' << e[1] << ',' << e[2] << ',' << e[3] << '\n';
      for(auto const &e : expected) std::cerr << "expected " << e[0] << ',' << e[1] << ',' << e[2] << ',' << e[3] << '\n';
      file.fail(label + " auxiliary events");
    }
    if(result != static_cast<bool>(expected_result) || targets[0].selected != final_selected || targets[0].changed != changed || targets[1].attackers != final_count || command != final_command || detectable != final_detectable || own.position != final_position || own.heading != final_heading) file.fail(label + " auxiliary state");
    if(kind == 0) { if(travel.search_remaining != final_timer || travel.threat != target(file.integer<uint8_t>() != 0)) file.fail(label + " travel state"); }
    else if(kind == 1) {
      bool const choice{file.integer<uint8_t>() != 0}; auto const direction{file.integer<int16_t>()};
      if(flee.turn_remaining != final_timer || flee.turn_right != choice || (actual.front()[2] < 0 ? -1 : 1) != direction) file.fail(label + " flee scratch overlap");
    } else if(kind == 2) {
      auto const previous{file.integer<int16_t>()}; bool const initialized{file.integer<uint8_t>() != 0}, left{file.integer<uint8_t>() != 0}; auto const distance{file.integer<int32_t>()}; bool const still_blocked{file.integer<uint8_t>() != 0}; auto const final_stagger{file.integer<uint16_t>()};
      if((skirt.previous ? static_cast<int>(*skirt.previous) : -1) != previous || skirt.initialized != initialized || skirt.turn_left != left || skirt.check_remaining != final_timer || skirt.distance != distance || obstruction != still_blocked || stagger != final_stagger) file.fail(label + " skirt state");
    } else {
      bool const burst{file.integer<uint8_t>() != 0}; auto const remaining{file.integer<int16_t>()};
      if(ram.bursting != burst || ram.burst_remaining != remaining || ram.search_remaining != final_timer) file.fail(label + " ram state");
    }
    auto const &state{random.snapshot()}; if(state.words != final_rng.words || state.first != final_rng.first || state.second != final_rng.second) file.fail(label + " RNG");
  }
  file.require_end(); for(auto const n : coverage) if(n != 768) file.fail("auxiliary coverage");
  std::cout << count << " original travel/flee/skirt/ram decisions agree\n";
  return EXIT_SUCCESS;
} catch(std::exception const &error) { std::cerr << error.what() << '\n'; return EXIT_FAILURE; }
