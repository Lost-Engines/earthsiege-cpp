#pragma once
#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include "resources/archive.h"
#include "resources/edition.h"

namespace earthsiege {

class resource_store {
  std::filesystem::path root_path;
  game_edition edition_{game_edition::original};
  std::vector<archive const *> overrides;
  std::map<std::string, archive, std::less<>> volumes;

public:
  explicit resource_store(std::filesystem::path const &root);
  auto read(std::string_view volume, std::string_view name) const->bytes;
  auto read_original(std::string_view volume, std::string_view name) const->bytes;
  std::optional<std::filesystem::path> external(std::string_view name) const;
  game_edition edition() const { return edition_; }
  auto archives() const->std::map<std::string, archive, std::less<>> const &;
  auto root() const->std::filesystem::path const &;
  static auto locate(std::filesystem::path const &program)->std::filesystem::path;
};

} // namespace earthsiege
