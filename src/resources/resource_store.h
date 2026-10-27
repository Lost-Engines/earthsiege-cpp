#pragma once
#include <filesystem>
#include <map>
#include <string>
#include <string_view>
#include "resources/archive.h"

namespace earthsiege {

class resource_store {
  std::filesystem::path root_path;
  std::map<std::string, archive, std::less<>> volumes;

public:
  explicit resource_store(std::filesystem::path const &root);
  auto read(std::string_view volume, std::string_view name) const->bytes;
  auto archives() const->std::map<std::string, archive, std::less<>> const &;
  auto root() const->std::filesystem::path const &;
  static auto locate(std::filesystem::path const &program)->std::filesystem::path;
};

} // namespace earthsiege
