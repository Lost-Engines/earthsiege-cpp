#pragma once
#include <cstdint>
#include <filesystem>
#include <map>
#include <string>
#include <string_view>
#include <vector>
#include "resources/binary_reader.h"

namespace earthsiege {

struct archive_entry {
  std::string name;
  uint32_t offset{};
  uint32_t size{};
  uint32_t timestamp{};
};

class archive {
  std::vector<uint8_t> data;
  std::map<std::string, archive_entry, std::less<>> catalogue;

public:
  explicit archive(std::filesystem::path const &path);
  explicit archive(std::vector<uint8_t> data, std::string label);
  auto entries() const->std::map<std::string, archive_entry, std::less<>> const &;
  auto find(std::string_view name) const->archive_entry const *;
  auto read(archive_entry const &entry) const->bytes;
};

} // namespace earthsiege
