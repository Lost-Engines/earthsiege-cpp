#include <array>
#include <cstdlib>
#include <iostream>
#include <vector>
#include "resources/binary_reader.h"
#include "simulation/herc_guard.h"
namespace {
using namespace earthsiege;
std::optional<size_t> optional_actor(int16_t value) { return value < 0 ? std::nullopt : std::make_optional(static_cast<size_t>(value)); }
}
int main(int argc, char **argv) try {
  if(argc != 2) return EXIT_FAILURE;
  auto const bytes{read_file(argv[1])}; binary_reader file{bytes}; auto const magic{file.take(8)};
  auto const version{std::string(magic.begin(), magic.end())};
  if(version != "ESHGD001" && version != "ESHGD002") file.fail("fixture version");
  auto const count{file.integer<uint32_t>()}; std::array<size_t, 2> coverage{};
  for(unsigned int trial{0}; trial != count; ++trial) {
    bool const driving{file.integer<uint8_t>() != 0}; ++coverage[driving];
    herc_guard_policy policy;
    if(version == "ESHGD002") {policy.expansion = file.integer<uint8_t>() != 0; policy.weapons_free = file.integer<uint8_t>() != 0;}
    std::array<engagement_state, 9> states; std::array<target_relationship, 9> targets;
    std::array<herc_combat_position, 9> positions; std::array<bool, 9> eligible;
    for(size_t i{0}; i != states.size(); ++i) {
      positions[i].position = {file.integer<int32_t>(), file.integer<int32_t>(), file.integer<int32_t>()}; positions[i].heading = file.integer<uint16_t>();
      eligible[i] = file.integer<uint8_t>() != 0; states[i].dead = file.integer<uint8_t>() != 0; states[i].disabled = file.integer<uint8_t>() != 0; states[i].unavailable = file.integer<uint8_t>() != 0; targets[i].attackers = file.integer<int16_t>();
    }
    policy.anchor = {file.integer<int32_t>(), file.integer<int32_t>(), file.integer<int32_t>()};
    auto &own{states[0]}; own.command = file.integer<int16_t>(); own.controller.target_search_remaining = file.integer<int16_t>();
    own.controller.type = driving ? behavior::driving_off_enemy : behavior::guarding;
    own.aimed_component = 123; int16_t tolerance{456};
    auto const ratio{file.integer<int16_t>()}; targets[0].selected = optional_actor(file.integer<int16_t>()); policy.designated = optional_actor(file.integer<int16_t>());
    policy.leader_player = file.integer<uint8_t>() != 0; policy.radar_active = file.integer<uint8_t>() != 0; policy.persistent_signature = file.integer<uint8_t>() != 0;
    bool detectable{file.integer<uint8_t>() != 0}; policy.group_has_objective = file.integer<uint8_t>() != 0;
    auto const component{file.integer<int16_t>()}; auto stagger{file.integer<uint16_t>()};
    bool const blocked{file.integer<uint8_t>() != 0}, faceoff{file.integer<uint8_t>() != 0}, completed{file.integer<uint8_t>() != 0};
    positions[8].view_offset = file.integer<int16_t>();
    std::vector<std::array<int32_t, 4>> expected(file.integer<uint8_t>()), actual;
    auto const expected_result{file.integer<uint16_t>()};
    for(auto &event : expected) for(auto &v : event) v = file.integer<int32_t>();
    auto const selected{optional_actor(file.integer<int16_t>())}, designated{optional_actor(file.integer<int16_t>())};
    std::array<int16_t, 9> counts; for(auto &v : counts) v = file.integer<int16_t>();
    bool const changed{file.integer<uint8_t>() != 0}; auto const final_timer{file.integer<int16_t>()}, final_component{file.integer<int16_t>()}, final_tolerance{file.integer<int16_t>()};
    bool const final_detectable{file.integer<uint8_t>() != 0}; auto const final_behavior{static_cast<behavior>(file.integer<uint8_t>())}; auto const final_clock{file.integer<int32_t>()}; auto const final_stagger{file.integer<uint16_t>()};
    auto const move{[&](movement_request const &request){actual.push_back({4, request.flags, request.speed, request.turn});}};
    bool result{false};
    if(!driving) {
      std::array<size_t, 8> const candidates{1,2,3,4,5,6,7,8};
      guard_with_herc(states, targets, 0, candidates, policy, detectable, ratio, {
        .position{[&](size_t actor){return positions[actor];}},
        .eligible{[&](size_t actor){actual.push_back({0, static_cast<int32_t>(actor), 0, 0}); return eligible[actor];}},
        .navigate{[&]{actual.push_back({1, 0, 0, 0});}},
        .centre_aim{[&]{actual.push_back({2, 0, 0, 0});}},
        .move_to{[&](vec3<int32_t> const &point){actual.push_back({3, point.x, point.y, point.z});}},
        .move{move},
        .choose_component{[&]{actual.push_back({5, 0, 0, 0}); return component;}},
        .enter{[&](behavior type){actual.push_back({6, static_cast<int32_t>(type), 0, 0}); enter_behavior(own.controller, type, stagger);}},
      });
    } else result = drive_off_enemy(positions[0], positions[8], policy.anchor, policy.designated, tolerance, {
      .obstruction_entry{[&]{actual.push_back({7, 0, 0, 0}); return blocked;}},
      .face_off{[&]{actual.push_back({8, 0, 0, 0}); return faceoff;}},
      .move{move},
      .aim{[&](int16_t facing){actual.push_back({10, facing, 0, 0});}},
      .target_completed{[&]{actual.push_back({9, 0, 0, 0}); return completed;}},
      .incapacitated{[&]{return own.dead || own.disabled || own.unavailable;}},
    });
    auto const label{std::to_string(trial)};
    if(actual != expected) {
      for(auto const &e : actual) std::cerr << "actual " << e[0] << ',' << e[1] << ',' << e[2] << ',' << e[3] << '\n';
      for(auto const &e : expected) std::cerr << "expected " << e[0] << ',' << e[1] << ',' << e[2] << ',' << e[3] << '\n';
      file.fail(label + " guard events");
    }
    if((driving && result != static_cast<bool>(expected_result)) || targets[0].selected != selected || targets[0].changed != changed || policy.designated != designated || own.controller.target_search_remaining != final_timer || own.aimed_component != final_component || tolerance != final_tolerance || detectable != final_detectable || own.controller.type != final_behavior || own.controller.reconsider_remaining != final_clock || stagger != final_stagger) file.fail(label + " guard state");
    for(size_t i{0}; i != counts.size(); ++i) if(targets[i].attackers != counts[i]) file.fail(label + " guard counts");
  }
  file.require_end(); if(coverage[0] != 1024 || coverage[1] != (version == "ESHGD001" ? 1024U : 0U)) file.fail("guard coverage");
  std::cout << count << " original guard/drive-off decisions agree\n";
  return EXIT_SUCCESS;
} catch(std::exception const &error) { std::cerr << error.what() << '\n'; return EXIT_FAILURE; }
