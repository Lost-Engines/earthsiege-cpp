#pragma once
#include <filesystem>
#include <iosfwd>
#include <optional>
#include "audio/music_settings.h"
#include "resources/edition.h"

namespace earthsiege {

struct launch_options {
  std::optional<std::filesystem::path> data_dir;
  std::filesystem::path save_dir;
  std::optional<game_edition> edition;
  std::optional<unsigned int> demo, inspect_demo;
  bool skip_intro{false};
  std::optional<music_device> music;
  std::optional<std::filesystem::path> rom_dir, soundfont;
  bool audio_setup{false};
  unsigned int scale{0};
  std::filesystem::path screenshot;
  unsigned int frame_limit{0};
  bool headless{false}, list_resources{false};
};

// --help prints to the supplied stream and returns an empty result.
auto parse_command_line(int argc, char const *const argv[], std::ostream &help)->std::optional<launch_options>;

} // namespace earthsiege
