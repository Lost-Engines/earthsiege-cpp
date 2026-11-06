#include "resource_store.h"
#include <array>
#include <algorithm>
#include <stdexcept>

namespace earthsiege {
namespace {

auto volume_directory(std::filesystem::path const &root)->std::filesystem::path {
  for(auto const &candidate : {root / "VOL", root / "vol", root}) {
    if(!std::filesystem::is_directory(candidate)) continue;
    for(auto const &entry : std::filesystem::directory_iterator{candidate}) {
      if(normalise_path(entry.path().filename().string()) == "simvol0.vol") return candidate;
    }
  }
  return {};
}

} // namespace

resource_store::resource_store(std::filesystem::path const &root) : root_path{root} {
  /// Accept the bundled assets or an existing DOS installation's VOL directory.
  auto const directory{volume_directory(root)};
  if(directory.empty()) throw std::runtime_error{"game data not found in " + root.string() + "; use the bundled assets directory or --data-dir PATH to an Earthsiege installation"};
  std::array constexpr required{"detect", "shell0", "simsound", "simvoice", "simvol0", "speech", "zones"};
  for(auto const &entry : std::filesystem::directory_iterator{directory}) {
    if(!entry.is_regular_file() || normalise_path(entry.path().extension().string()) != ".vol") continue;
    auto const name{normalise_path(entry.path().stem().string())};
    if(!volumes.try_emplace(name, entry.path()).second) throw std::runtime_error{"duplicate game volume: " + name};
  }
  if(volumes.contains("pshell2") && volumes.contains("simpatch") && volumes.contains("pzones1")) {
    edition_ = game_edition::expansion;
    // The original selects language volumes explicitly. Default to English;
    // alphabetical directory order must not substitute French/German speech.
    for(auto const &[name, volume] : volumes) {
      if(name == "simvoicf" || name == "simvoicg" || name == "fspeech" || name == "gspeech" || name == "pfspeech" || name == "pgspeech") continue;
      overrides.push_back(&volume);
    }
    std::stable_sort(overrides.begin(), overrides.end(), [](auto a, auto b){ return a->priority() > b->priority(); });
  }
  for(auto const name : required) {
    if(!volumes.contains(name)) throw std::runtime_error{"missing game volume " + std::string{name} + " in " + directory.string()};
  }
}

auto resource_store::read(std::string_view volume, std::string_view name) const->bytes {
  auto const found{volumes.find(normalise_path(volume))};
  if(found == volumes.end()) throw std::runtime_error{"unknown game volume: " + std::string{volume}};
  if(edition_ == game_edition::expansion) {
    auto const usage{found->second.usage_mask()};
    for(auto const *candidate : overrides) if(candidate->usage_mask() & usage) {
      if(auto const *entry{candidate->find(name)}) return candidate->read(*entry);
    }
  }
  return read_original(volume, name);
}

auto resource_store::read_original(std::string_view volume, std::string_view name) const->bytes {
  auto const found{volumes.find(normalise_path(volume))};
  if(found == volumes.end()) throw std::runtime_error{"unknown game volume: " + std::string{volume}};
  auto const *entry{found->second.find(name)};
  if(!entry) throw std::runtime_error{"resource not found: " + std::string{volume} + ":" + std::string{name}};
  return found->second.read(*entry);
}

std::optional<std::filesystem::path> resource_store::external(std::string_view name) const {
  auto relative{std::filesystem::path{normalise_path(name)}};
  if(relative.is_absolute())throw std::runtime_error{"absolute game resource path"};
  for(auto const &part:relative)if(part=="..")throw std::runtime_error{"parent in game resource path"};
  auto directory{root_path};if(normalise_path(directory.filename().string())=="vol")directory=directory.parent_path();
  for(auto const &part:relative){
    if(!std::filesystem::is_directory(directory))return {};
    bool found{};
    for(auto const &entry:std::filesystem::directory_iterator{directory})if(normalise_path(entry.path().filename().string())==part.string()){directory=entry.path();found=true;break;}
    if(!found)return {};
  }
  if(std::filesystem::is_regular_file(directory))return directory;
  return {};
}

auto resource_store::archives() const->std::map<std::string, archive, std::less<>> const & { return volumes; }
auto resource_store::root() const->std::filesystem::path const & { return root_path; }

auto resource_store::locate(std::filesystem::path const &program)->std::filesystem::path {
  for(auto const &candidate : {std::filesystem::path{"assets"}, program.parent_path() / "assets", program.parent_path().parent_path() / "assets"}) {
    if(!volume_directory(candidate).empty()) return candidate;
  }
  return "assets";
}

} // namespace earthsiege
