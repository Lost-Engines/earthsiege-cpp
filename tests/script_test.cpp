#include <array>
#include <bit>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include "resources/input_tape.h"
#include "resources/mission_compiler.h"
#include "resources/mission_script.h"
#include "resources/resource_store.h"
#include "simulation/random_state.h"

namespace {

struct fixture_values {
  std::vector<uint8_t> data;
  void word(int16_t value) {
    data.push_back(static_cast<uint8_t>(value));
    data.push_back(static_cast<uint8_t>(static_cast<uint16_t>(value) >> 8));
  }
  template<typename R>
  void words(R const &values) { for(auto const value : values) word(value); }
  void dword(int32_t value) {
    for(unsigned int byte{}; byte < 4; ++byte) data.push_back(static_cast<uint8_t>(static_cast<uint32_t>(value) >> (byte * 8)));
  }
  auto checksum()->uint32_t {
    auto const value{earthsiege::crc32(data)};
    data.clear();
    return value;
  }
};

auto signatures(earthsiege::mission_script const &script)->std::array<uint32_t, 10> {
  /// Compare named C++ fields against original compiler in-memory records.
  std::array<uint32_t, 10> result{};
  fixture_values values;
  values.word(std::to_underlying(script.header.terrain));
  values.word(script.header.zone);
  values.word(script.header.difficulty);
  values.words(script.header.settings);
  values.word(script.header.world_variant);
  values.word(script.header.palette_variant);
  result[0] = values.checksum();
  for(auto const &point : script.points) { values.dword(point.x); values.dword(point.y); values.dword(point.z); }
  result[1] = values.checksum();
  values.words(script.headings);
  result[2] = values.checksum();
  for(auto const &route : script.routes) { values.word(static_cast<int16_t>(route.points.size())); values.words(route.points); }
  result[3] = values.checksum();
  for(auto const &area : script.areas) { values.word(area.kind); values.word(area.first_point); values.word(area.second_point_or_radius); }
  result[4] = values.checksum();
  for(auto const &trigger : script.triggers) {
    values.word(trigger.mode); values.word(trigger.test); values.words(trigger.areas); values.word(trigger.counter); values.word(trigger.counter_operation);
  }
  result[5] = values.checksum();
  for(auto const &herc : script.hercs) {
    values.words(herc.field_mask); values.word(herc.chassis); values.words(herc.weapons);
    values.word(herc.position); values.word(herc.heading); values.word(herc.counter); values.word(herc.counter_operation);
    values.words(herc.ammunition); values.word(herc.skill); values.word(herc.trigger_a); values.word(herc.trigger_b); values.word(herc.initial_condition);
  }
  result[6] = values.checksum();
  for(auto const &flyer : script.flyers) {
    values.words(flyer.field_mask); values.word(flyer.position); values.word(flyer.heading); values.word(flyer.type);
    values.word(flyer.counter); values.word(flyer.counter_operation); values.word(flyer.trigger_a); values.word(flyer.trigger_b); values.word(flyer.initial_value);
  }
  result[7] = values.checksum();
  for(auto const &base : script.bases) {
    values.word(base.type); values.word(base.position); values.word(base.heading); values.word(base.counter);
    values.word(base.counter_operation); values.word(base.trigger_a); values.word(base.trigger_b); values.word(base.initial_value);
  }
  result[8] = values.checksum();
  for(auto const &group : script.groups) {
    values.words(group.field_mask); values.word(std::to_underlying(group.kind)); values.word(group.formation); values.word(group.argument);
    values.word(group.position); values.word(group.heading); values.word(group.route); values.words(group.members);
    values.word(group.mode); values.word(std::to_underlying(group.target_type)); values.word(group.target); values.word(group.trigger); values.words(group.extra);
  }
  result[9] = values.checksum();
  return result;
}

} // namespace

auto main(int argc, char *argv[])->int try {
  if(argc != 3) throw std::runtime_error{"expected fixture and asset roots"};
  std::filesystem::path const root{argv[1]};
  earthsiege::resource_store const resources{argv[2]};
  std::ifstream fixture{root / "script_checks.tsv"};
  std::string name;
  size_t consumed{};
  unsigned int checked{};
  while(fixture >> name >> consumed) {
    std::array<uint32_t, 10> expected{};
    for(auto &value : expected) fixture >> std::hex >> value >> std::dec;
    uint32_t expected_random{};
    fixture >> std::hex >> expected_random >> std::dec;
    auto const data{earthsiege::read_file(root / "fixtures/scripts" / name)};
    auto const script{earthsiege::read_mission_script(data)};
    if(script.consumed != consumed) throw std::runtime_error{"handoff extent differs from original: " + name};
    auto const actual{signatures(script)};
    for(unsigned int table{}; table < expected.size(); ++table) {
      if(actual[table] != expected[table]) throw std::runtime_error{"handoff table " + std::to_string(table) + " differs from native compiler state: " + name};
    }
    earthsiege::random_state::state initial;
    if(!name.starts_with("zero/")) {
      for(unsigned int index{}; index < initial.words.size(); ++index) initial.words[index] = static_cast<uint16_t>((index * 7919 + 12345) & 32767);
      initial.first = 31;
    }
    earthsiege::random_state random{initial};
    std::array<int16_t, 100> flags{};
    if(name.starts_with("flags_one/")) flags.fill(1);
    if(name.starts_with("flags_pattern/")) {
      for(unsigned int index{}; index < flags.size(); ++index) flags[index] = static_cast<int16_t>(index % 5);
    }
    auto const source{resources.read("zones", "msn/" + std::filesystem::path{name}.stem().string() + ".msn")};
    auto const compiled{earthsiege::compile_mission(source, flags, random, 0)};
    auto const generated{signatures(compiled.script)};
    for(unsigned int table{}; table < expected.size(); ++table) {
      if(generated[table] != expected[table]) throw std::runtime_error{"compiled table " + std::to_string(table) + " differs from native compiler: " + name};
    }
    fixture_values final_random;
    for(auto const value : random.snapshot().words) final_random.word(std::bit_cast<int16_t>(value));
    final_random.data.push_back(static_cast<uint8_t>(random.snapshot().first));
    final_random.data.push_back(static_cast<uint8_t>(random.snapshot().second));
    if(final_random.checksum() != expected_random) throw std::runtime_error{"compiler random state differs from original: " + name};
    ++checked;
  }
  if(checked != 300) throw std::runtime_error{"incomplete native handoff fixtures"};
  std::array constexpr frame_counts{2308u, 2951u, 2470u, 1571u};
  std::array constexpr mouse_counts{1332u, 983u, 42u, 370u};
  unsigned int total_frames{};
  for(unsigned int index{}; index < frame_counts.size(); ++index) {
    auto const data{earthsiege::read_file(std::filesystem::path{argv[2]} / "TAPES" / ("DEMO" + std::to_string(index + 1) + ".TAP"))};
    auto const tape{earthsiege::read_input_tape(data)};
    auto const script{earthsiege::read_mission_script(tape.starting_files[std::to_underlying(earthsiege::tape_file::script)])};
    if(tape.frames.size() != frame_counts[index] || script.groups.empty()) throw std::runtime_error{"incorrect original tape state"};
    fixture_values values;
    unsigned int mouse_events{};
    for(auto const &frame : tape.frames) {
      values.word(static_cast<int16_t>(frame.command));
      values.word(frame.primary_axes.x); values.word(frame.primary_axes.y);
      values.word(frame.secondary_axes.x); values.word(frame.secondary_axes.y);
      values.word(static_cast<int16_t>(frame.delta)); values.word(static_cast<int16_t>(frame.buttons));
      values.dword(static_cast<int32_t>(frame.mouse.size()));
      for(auto const &event : frame.mouse) {
        values.dword(event.position.x); values.dword(event.position.y); values.word(static_cast<int16_t>(event.kind)); values.dword(static_cast<int32_t>(event.timestamp));
        ++mouse_events;
      }
    }
    if(mouse_events != mouse_counts[index] || values.checksum() != earthsiege::crc32(earthsiege::bytes{data}.subspan(tape.stream_offset))) throw std::runtime_error{"tape inputs differ from original bytes"};
    total_frames += static_cast<unsigned int>(tape.frames.size());
  }
  std::cout << checked << " original mission handoffs and " << total_frames << " recorded input frames agree" << std::endl;
  return EXIT_SUCCESS;
} catch(std::exception const &error) {
  std::cerr << error.what() << std::endl;
  return EXIT_FAILURE;
}
