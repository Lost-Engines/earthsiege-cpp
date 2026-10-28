#include <array>
#include <cstdlib>
#include <iostream>
#include <vector>
#include "resources/binary_reader.h"
#include "simulation/herc_strategy.h"
namespace {
using namespace earthsiege;
random_state::state read_random(binary_reader &file) {
  random_state::state state; for(auto &v : state.words) v = file.integer<uint16_t>();
  state.first = file.integer<uint8_t>(); state.second = file.integer<uint8_t>(); return state;
}
std::optional<size_t> optional_actor(int16_t value) { return value < 0 ? std::nullopt : std::make_optional(static_cast<size_t>(value)); }
}
int main(int argc, char **argv) try {
  if(argc != 2) return EXIT_FAILURE;
  auto const bytes{read_file(argv[1])}; binary_reader file{bytes}; auto const magic{file.take(8)};
  if(std::string(magic.begin(), magic.end()) != "ESHST001") file.fail("fixture version");
  auto const count{file.integer<uint32_t>()};
  std::array<size_t, 8> const group{0, 1, 2, 3, 4, 5, 6, 7};
  for(unsigned int trial{0}; trial != count; ++trial) {
    auto const actor{file.integer<uint8_t>()}; herc_strategy_policy policy; policy.order = file.integer<int16_t>();
    std::array<engagement_state, 10> states; std::array<target_relationship, 10> targets;
    auto &state{states[actor]}; state.command = file.integer<int16_t>(); state.attack_requested = file.integer<uint8_t>() != 0; state.aimed_component = 123;
    targets[actor].selected = optional_actor(file.integer<int16_t>()); auto const chosen{optional_actor(file.integer<int16_t>())};
    bool const retreat{file.integer<uint8_t>() != 0}; auto const component{file.integer<int16_t>()};
    policy.radar_active = file.integer<uint8_t>() != 0; bool detectable{file.integer<uint8_t>() != 0}; policy.ecm_remaining = file.integer<int16_t>();
    auto const limit{file.integer<int16_t>()}; auto stagger{file.integer<uint16_t>()};
    std::array<bool, 10> player{}; std::array<int16_t, 10> strengths{}; std::array<combat_actor_kind, 10> kinds;
    for(size_t i{0}; i != states.size(); ++i) {
      states[i].controller.type = static_cast<behavior>(file.integer<uint8_t>());
      states[i].dead = file.integer<uint8_t>() != 0; states[i].disabled = file.integer<uint8_t>() != 0;
      player[i] = file.integer<uint8_t>() != 0; strengths[i] = file.integer<int16_t>(); kinds[i] = static_cast<combat_actor_kind>(file.integer<uint8_t>());
      targets[i].attackers = 10;
    }
    policy.leader_player = player.front(); random_state random{read_random(file)};
    std::vector<std::array<int16_t, 2>> expected(file.integer<uint8_t>()), actual;
    for(auto &event : expected) { event[0] = file.integer<uint8_t>(); event[1] = file.integer<int16_t>(); }
    auto const selected{optional_actor(file.integer<int16_t>())}; bool const changed{file.integer<uint8_t>() != 0};
    std::array<int16_t, 10> counts; for(auto &v : counts) v = file.integer<int16_t>();
    bool const final_requested{file.integer<uint8_t>() != 0}, final_detectable{file.integer<uint8_t>() != 0}; auto const final_component{file.integer<int16_t>()};
    auto const type{static_cast<behavior>(file.integer<uint8_t>())}; auto const clock{file.integer<int32_t>()}; auto const final_stagger{file.integer<uint16_t>()}; bool const first{file.integer<uint8_t>() != 0}; auto const final_rng{read_random(file)};
    auto const enter{[&](behavior next){ actual.push_back({6, static_cast<int16_t>(next)}); enter_behavior(state.controller, next, stagger); }};
    reconsider_herc_attack(states, targets, actor, group, policy, detectable, {
      .select{[&](target_selection_flags flags){ actual.push_back({0, static_cast<int16_t>(flags)}); return chosen; }},
      .target_kind{[&](size_t target){ return kinds[target]; }},
      .alert_follower{[&](size_t follower){ actual.push_back({1, static_cast<int16_t>(follower)}); }},
      .resume_orders{[&]{ actual.push_back({4, 0}); assign_target(targets, actor, std::nullopt); enter(initial_herc_behavior(player[actor], policy.order, limit)); }},
      .retreat{[&]{ actual.push_back({2, 0}); return retreat; }},
      .compare_strength{[&]{ actual.push_back({5, 0}); return compare_herc_strength(strengths[actor], strengths[*targets[actor].selected], kinds[*targets[actor].selected], random); }},
      .choose_component{[&]{ actual.push_back({3, 0}); return component; }},
      .enter{enter},
    });
    auto const label{std::to_string(trial)};
    if(actual != expected || targets[actor].selected != selected || targets[actor].changed != changed || state.attack_requested != final_requested || detectable != final_detectable || state.aimed_component != final_component || state.controller.type != type || state.controller.reconsider_remaining != clock || stagger != final_stagger || state.controller.first_entry != first) file.fail(label + " strategy events/state");
    for(size_t i{0}; i != counts.size(); ++i) if(targets[i].attackers != counts[i]) file.fail(label + " target counts");
    auto const &rng{random.snapshot()}; if(rng.words != final_rng.words || rng.first != final_rng.first || rng.second != final_rng.second) file.fail(label + " strategy RNG");
  }
  file.require_end(); if(count != 2560) file.fail("strategy coverage");
  std::cout << count << " original HERC strategy transitions agree\n";
  return EXIT_SUCCESS;
} catch(std::exception const &error) { std::cerr << error.what() << '\n'; return EXIT_FAILURE; }
