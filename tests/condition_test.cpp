#include <array>
#include <bit>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>
#include "resources/mission_compiler.h"
#include "resources/mission_conditions.h"
#include "resources/resource_store.h"
#include "simulation/random_state.h"

namespace {

auto native_words(earthsiege::mission_condition const &condition)->std::array<int16_t, 7> {
  std::array<int16_t, 7> result{condition.id, condition.parent, static_cast<int16_t>(condition.rule.index()), 0, 0, 0, condition.scratch};
  if(auto const *flag{std::get_if<earthsiege::flag_test>(&condition.rule)}) {
    result[3] = flag->flag;
    result[4] = static_cast<int16_t>(flag->comparison);
    result[5] = flag->expected;
  } else if(auto const *draw{std::get_if<earthsiege::random_draw>(&condition.rule)}) {
    result[3] = draw->limit;
    result[4] = draw->source_result;
    result[5] = draw->reserved;
  } else if(auto const *interval{std::get_if<earthsiege::random_interval>(&condition.rule)}) {
    result[3] = std::bit_cast<int16_t>(interval->first);
    result[4] = std::bit_cast<int16_t>(interval->last);
    result[5] = interval->reserved;
  } else if(auto const *group{std::get_if<earthsiege::random_group>(&condition.rule)}) {
    result[3] = group->group;
    result[4] = group->reserved;
    result[5] = group->limit;
  }
  if(condition.result) result[4] = *condition.result;
  return result;
}

} // namespace

auto main(int argc, char *argv[])->int try {
  if(argc != 4) throw std::runtime_error{"expected assets and original condition/RNG fixtures"};
  earthsiege::resource_store const resources{argv[1]};
  std::ifstream fixture{argv[2]};
  std::string name;
  std::string seed;
  unsigned int count{};
  unsigned int cases{};
  while(fixture >> name >> seed >> count) {
    earthsiege::binary_reader file{resources.read("zones", "msn/" + name), name};
    if(file.integer<uint16_t>() != 5) throw std::runtime_error{"unexpected source mission revision"};
    auto const conditions{earthsiege::read_conditions(file)};
    earthsiege::random_state::state initial;
    if(seed == "seeded") {
      for(unsigned int index{}; index < initial.words.size(); ++index) initial.words[index] = static_cast<uint16_t>((index * 7919 + 12345) & 32767);
      initial.first = 31;
    }
    earthsiege::random_state random{initial};
    std::array<int16_t, 100> const flags{};
    auto const active{earthsiege::evaluate_conditions(conditions, flags, random)};
    if(active.size() != count) throw std::runtime_error{"condition count differs from original: " + name};
    std::vector<std::array<int16_t, 7>> expected_complete;
    for(unsigned int index{}; index < count; ++index) {
      std::array<int16_t, 7> expected{};
      for(auto &word : expected) fixture >> word;
      expected_complete.push_back(expected);
      auto actual{native_words(active[index])};
      if(expected[2] == 3) actual[6] = expected[6];                        // group scratch binds during later table compilation
      if(actual != expected) throw std::runtime_error{"condition differs from original: " + name};
    }
    earthsiege::random_state complete_random{initial};
    auto const complete{earthsiege::compile_mission(resources.read("zones", "msn/" + name), flags, complete_random, 0)};
    for(unsigned int index{}; index < count; ++index) {
      if(native_words(complete.conditions.at(index)) != expected_complete[index]) throw std::runtime_error{"late condition binding differs from original: " + name};
    }
    ++cases;
  }
  if(cases != 150) throw std::runtime_error{"incomplete condition fixtures"};
  std::ifstream random_fixtures{argv[3]};
  int16_t advances{};
  unsigned int draws{};
  uint32_t expected_crc{};
  unsigned int rng_cases{};
  while(random_fixtures >> advances >> draws >> std::hex >> expected_crc >> std::dec) {
    earthsiege::random_state random{advances};
    std::vector<uint8_t> result;
    for(unsigned int index{}; index < draws; ++index) {
      auto const value{std::bit_cast<uint16_t>(random.next())};
      result.push_back(static_cast<uint8_t>(value));
      result.push_back(static_cast<uint8_t>(value >> 8));
    }
    if(earthsiege::crc32(result) != expected_crc) throw std::runtime_error{"random stream differs from original"};
    ++rng_cases;
  }
  earthsiege::random_state negative_bound;
  if(negative_bound.bounded(-5) != 2) throw std::runtime_error{"signed random bound differs from original"};
  earthsiege::random_state zero_bound;
  earthsiege::random_state unchanged;
  if(zero_bound.bounded(0) != 0 || zero_bound.next() != unchanged.next()) throw std::runtime_error{"zero random bound advanced the generator"};
  if(rng_cases != 9) throw std::runtime_error{"incomplete random fixtures"};
  std::cout << cases << " native condition comparisons and " << rng_cases << " original random streams agree" << std::endl;
  return EXIT_SUCCESS;
} catch(std::exception const &error) {
  std::cerr << error.what() << std::endl;
  return EXIT_FAILURE;
}
