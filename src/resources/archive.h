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
  uint32_t offset{0};
  uint32_t size{0};
  uint32_t timestamp{0};
};

class archive {
  std::vector<uint8_t> data;
  uint32_t usage_{0};
  uint8_t priority_{0};
  std::map<std::string, archive_entry, std::less<>> catalogue;

public:
  explicit archive(std::filesystem::path const &path);
  explicit archive(std::vector<uint8_t> data, std::string label);
  uint32_t usage_mask() const {
    return usage_;
  }
  uint8_t priority() const {
    return priority_;
  }
  auto entries() const->std::map<std::string, archive_entry, std::less<>> const &;
  auto find(std::string_view name) const->archive_entry const *;
  auto read(archive_entry const &entry) const->bytes;
};

} // namespace earthsiege
