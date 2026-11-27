#include "command_line.h"
#include <algorithm>
#include <boost/program_options.hpp>
#include <cstdint>
#include <limits>
#include <ostream>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>

namespace earthsiege {
namespace {

auto path_value(auto &destination) {
  return boost::program_options::value<std::string>()->value_name("PATH")->notifier([&destination](std::string const &path){
    destination = std::filesystem::path{path};
  });
}

auto count_value(char const *name, auto &destination) {
  // Boost accepts a leading minus for unsigned targets. Check before narrowing.
  return boost::program_options::value<int64_t>()->value_name("N")->notifier([name, &destination](int64_t value){
    if(!std::in_range<unsigned int>(value)) {
      throw std::invalid_argument{std::string{name} + " expects an integer from 0 to " + std::to_string(std::numeric_limits<unsigned int>::max())};
    }
    destination = static_cast<unsigned int>(value);
  });
}

} // namespace

auto parse_command_line(int argc, char const *const argv[], std::ostream &help)->std::optional<launch_options> {
  launch_options result;
  boost::program_options::options_description options{"Options", 110};
  options.add_options()
    ("help", "Show the available options")
    ("data-dir", path_value(result.data_dir), "Original game data (default: bundled assets)")
    ("original", "Start the original game directly")
    ("expansion", "Start the expansion directly")
    ("save-dir", path_value(result.save_dir), "Override the user save directory")
    ("demo", count_value("--demo", result.demo), "Play recording 1-4 (default: original game)")
    ("inspect-demo", count_value("--inspect-demo", result.inspect_demo), "Take the controls in the starting mission of recording 1-4")
    ("skip-intro", boost::program_options::bool_switch(&result.skip_intro), "Skip opening and pilot-death cinematics")
    ("music", boost::program_options::value<std::string>()->value_name("NAME"), "Choose opl2, opl3, sc55, scc1a, mt32, mt32-mpu or soundfont")
    ("rom-dir", path_value(result.rom_dir), "Override automatic Roland ROM discovery")
    ("soundfont", path_value(result.soundfont), "SoundFont file for General MIDI")
    ("audio-setup", boost::program_options::bool_switch(&result.audio_setup), "Open the music-device selector")
    ("scale", count_value("--scale", result.scale), "Integer window scale (default: largest fit)");

  boost::program_options::options_description capture{"Capture and inspection", 110};
  capture.add_options()
    ("screenshot", path_value(result.screenshot), "Save an unscaled PPM frame")
    ("frames", count_value("--frames", result.frame_limit), "Exit after N frames")
    ("headless", boost::program_options::bool_switch(&result.headless), "Render without creating a window")
    ("list-resources", boost::program_options::bool_switch(&result.list_resources), "Print original resource sizes and CRCs");
  options.add(capture);

  auto parsed{boost::program_options::command_line_parser(argc, argv).options(options)
    .style(boost::program_options::command_line_style::unix_style & ~boost::program_options::command_line_style::allow_guessing)
    .positional(boost::program_options::positional_options_description{}).run()};
  // Launch scripts can supply defaults first, then override them on the command line.
  std::ranges::reverse(parsed.options);
  std::set<std::string> seen;
  std::erase_if(parsed.options, [&](auto const &option){
    auto const &name{option.string_key};
    return !seen.insert(name == "original" || name == "expansion" ? "edition" : name).second;
  });
  boost::program_options::variables_map arguments;
  boost::program_options::store(parsed, arguments);
  if(arguments.contains("help")) {
    help << "Earthsiege C++\nUsage: earthsiege [options]\n\n" << options;
    return {};
  }
  boost::program_options::notify(arguments);
  if(arguments.contains("original")) result.edition = game_edition::original;
  if(arguments.contains("expansion")) result.edition = game_edition::expansion;
  if(arguments.contains("music")) result.music = parse_music_device(arguments["music"].as<std::string>());
  if(result.demo && (*result.demo < 1 || *result.demo > 4)) throw std::invalid_argument{"--demo must be between 1 and 4"};
  if(result.inspect_demo && (*result.inspect_demo < 1 || *result.inspect_demo > 4)) throw std::invalid_argument{"--inspect-demo must be between 1 and 4"};
  if(result.demo && result.inspect_demo) throw std::invalid_argument{"--demo and --inspect-demo cannot be used together"};
  return result;
}

} // namespace earthsiege
