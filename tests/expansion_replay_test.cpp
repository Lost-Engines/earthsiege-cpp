#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <vector>
#include "shell/demo_playback.h"
using earthsiege::binary_reader;
using earthsiege::demo_playback;
using earthsiege::framebuffer;
using earthsiege::preferences;
using earthsiege::read_file;
using earthsiege::resource_store;

namespace {
uint32_t cockpit_rgb_crc(earthsiege::mission_view const &view, earthsiege::rectangle area) {
  std::vector<uint8_t> rgb;
  for(int y{area.top}; y <= area.bottom; ++y) {
    for(int x{area.left}; x <= area.right; ++x) {
      auto const index{view.image().pixels()[y * earthsiege::screen_width + x]};
      auto const colour{view.colours().colours[index]};
      rgb.push_back(static_cast<uint8_t>(colour >> 16));
      rgb.push_back(static_cast<uint8_t>(colour >> 8));
      rgb.push_back(static_cast<uint8_t>(colour));
    }
  }
  return earthsiege::crc32(rgb);
}
}
int main(int argc, char **argv) try {
  if(argc != 4) throw std::runtime_error{"Usage: expansion_replay_test DATA FIXTURE SAVE_DIRECTORY"};
  auto const bytes{read_file(argv[2])};
  binary_reader expected{bytes};
  auto magic{expected.take(8)};
  if(std::string(magic.begin(), magic.end()) != "ESPATH02") expected.fail("demo path fixture version");
  auto const demo{expected.integer<uint32_t>()};
  auto const frames{expected.integer<uint32_t>()};
  auto const packets{expected.integer<uint32_t>()};
  auto const prefs{expected.take(expected.integer<uint16_t>())};
  resource_store resources{argv[1]};
  preferences settings{prefs, true, resources.edition()};
  demo_playback replay{resources, demo, settings, std::filesystem::path{argv[3]} / ("demo" + std::to_string(demo))};
  auto const present{[&](unsigned, framebuffer const &, framebuffer const *){
    replay.view().take_audio();
    replay.view().take_pan_frames();
    return true;
  }};
  unsigned startup{0};
  while(replay.view().starting() && !replay.done()) {
    if(++startup > 20000) expected.fail("demo startup did not finish");
    replay.step(present);
  }
  for(uint32_t frame{0}; frame < frames; ++frame) {
    if(replay.done()) expected.fail("demo ended before original frame " + std::to_string(frame));
    auto check{[&](auto actual, auto original, char const *field){
      if(actual != original) expected.fail("demo " + std::to_string(demo) + " frame " + std::to_string(frame) + " " + field + ": native " + std::to_string(actual) + ", original " + std::to_string(original));
    }};
    auto const &random{replay.view().simulation().random_snapshot()};
    for(auto value : random.words) check(value, expected.integer<uint16_t>(), "gameplay random word");
    check(random.first, expected.integer<uint8_t>(), "gameplay random first tap");
    check(random.second, expected.integer<uint8_t>(), "gameplay random second tap");
    replay.step(present);
    check(replay.frames_played(), frame + 1, "simulation count");
    check(replay.packets_read(), expected.integer<uint32_t>(), "input packets");
    if(demo == 2 && frame == 1) {
      // Lossless DOS capture, forward Apocalypse cockpit immediately after the lift.
      auto const &view{replay.view()};
      check(cockpit_rgb_crc(view, {16, 140, 71, 174}), 0x8f786c3cu, "left weapon bank");
      check(cockpit_rgb_crc(view, {248, 140, 303, 174}), 0x8a433a21u, "right weapon bank");
      check(cockpit_rgb_crc(view, {124, 122, 194, 165}), 0xf594eeccu, "scanner contacts");
      check(cockpit_rgb_crc(view, {106, 177, 215, 184}), 0x645f8041u, "scanner and mode controls");
      check(cockpit_rgb_crc(view, {133, 188, 215, 196}), 0x05751ae7u, "range and target controls");
    }
    if(demo == 2 && replay.packets_read() == 803) {
      // At 100 seconds the HERC is turning. The scanner uses the last world draw.
      check(cockpit_rgb_crc(replay.view(), {124, 122, 194, 165}), 0x5e1e710eu, "turning scanner contacts");
    }
    if(demo == 4) {
      // DOS capture of the Apocalypse at equal allocation, then either side of the rear 0-to-20 transfer.
      auto const &view{replay.view()};
      auto const packet{replay.packets_read()};
      if(packet == 107) check(cockpit_rgb_crc(view, {202, 3, 239, 20}), 0x8a630784u, "front and rear shields");
      if(packet == 820) check(cockpit_rgb_crc(view, {202, 12, 239, 20}), 0x22f219d6u, "empty rear shield");
      if(packet == 856) check(cockpit_rgb_crc(view, {202, 3, 239, 20}), 0xb883b604u, "rebalanced front and rear shields");
    }
    auto const &motion{replay.view().simulation().player().movement()};
    for(auto value : {motion.position.x, motion.position.y, motion.position.z}) check(value, expected.integer<int32_t>(), "position");
    for(auto value : {motion.angles.x, motion.angles.y, motion.angles.z}) check(value, expected.integer<uint16_t>(), "angle");
    check(motion.throttle, expected.integer<int16_t>(), "throttle");
    check(motion.gait_rate, expected.integer<int16_t>(), "gait");
    check(motion.steering_rate, expected.integer<int16_t>(), "steering");
    check(motion.playback_rate, expected.integer<int16_t>(), "animation rate");
  }
  expected.require_end();
  if(!replay.done() || replay.packets_read() != packets) expected.fail("demo did not end at the original packet");
  std::cout << "Expansion demo " << demo << ": " << frames << " player states and gameplay RNG agree exactly with DOSBox; " << packets << " packets, " << replay.modal_packets_read() << " modal packets\n";
  return EXIT_SUCCESS;
} catch(std::exception const &e) {
  std::cerr << e.what() << '\n';
  return EXIT_FAILURE;
}
