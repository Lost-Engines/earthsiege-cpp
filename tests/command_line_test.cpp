#include <cstdlib>
#include <initializer_list>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include "command_line.h"

namespace {
void require(bool value, char const *reason) {
  if(!value) throw std::runtime_error{reason};
}

auto parse(std::initializer_list<char const *> options, std::ostream &help) {
  std::vector<char const *> argv{"earthsiege"};
  argv.insert(argv.end(), options);
  int const argc{static_cast<int>(argv.size())};
  argv.push_back(nullptr);
  return earthsiege::parse_command_line(argc, argv.data(), help);
}

void reject(std::initializer_list<char const *> options) {
  std::ostringstream help;
  try {
    parse(options, help);
  } catch(std::exception const &) {
    return;
  }
  throw std::runtime_error{std::string{"unexpectedly accepted "} + *options.begin()};
}
}

int main() try {
  std::ostringstream help;
  auto const defaults{parse({}, help).value()};
  require(!defaults.edition && !defaults.demo && !defaults.inspect_demo && !defaults.data_dir && !defaults.music &&
          !defaults.skip_intro && !defaults.audio_setup && defaults.scale == 0, "default launch");
  auto const chosen{parse({"--original", "--expansion", "--original", "--demo=3", "--scale", "2", "--scale=5",
                           "--skip-intro", "--skip-intro", "--audio-setup", "--music", "roland-sc55",
                           "--data-dir=Game Data", "--save-dir", "Pilot Saves", "--rom-dir", "Roland ROMs",
                           "--soundfont=General MIDI.sf2"}, help).value()};
  require(chosen.edition == earthsiege::game_edition::original && chosen.demo == 3 && chosen.scale == 5 &&
          chosen.skip_intro && chosen.audio_setup && chosen.music == earthsiege::music_device::sc55, "edition, overrides and music");
  require(chosen.data_dir == std::filesystem::path{"Game Data"} && chosen.save_dir == "Pilot Saves" &&
          chosen.rom_dir == std::filesystem::path{"Roland ROMs"} && chosen.soundfont == std::filesystem::path{"General MIDI.sf2"}, "paths with spaces");
  auto const expansion{parse({"--original", "--expansion", "--inspect-demo=4"}, help).value()};
  require(expansion.edition == earthsiege::game_edition::expansion && expansion.inspect_demo == 4, "expansion inspection");
  require(!parse({"--help"}, help) && help.str().contains("--inspect-demo"), "generated help");
  require(help.str().contains("--headless"), "capture options appear in help");
  auto const capture{parse({"--headless", "--frames=2", "--screenshot", "Test Frame.ppm", "--list-resources"}, help).value()};
  require(capture.headless && capture.frame_limit == 2 && capture.screenshot == "Test Frame.ppm" && capture.list_resources, "capture options");

  reject({"--scale", "-1"});
  reject({"--scale=4294967296"});
  reject({"--scale", "2.5"});
  reject({"--demo=0"});
  reject({"--inspect-demo", "5"});
  reject({"--demo", "1", "--inspect-demo", "2"});
  reject({"--music", "unknown"});
  reject({"--rom-dir"});
  reject({"--original=true"});
  reject({"--expan"});
  reject({"stray"});
  reject({"--resolution=640x400"});
  return EXIT_SUCCESS;
} catch(std::exception const &error) {
  std::cerr << error.what() << '\n';
  return EXIT_FAILURE;
}
