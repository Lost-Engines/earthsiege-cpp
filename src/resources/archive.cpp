#include "archive.h"
#include <algorithm>
#include <array>
#include <stdexcept>
#include <utility>

namespace earthsiege {

archive::archive(std::filesystem::path const &path) : archive{read_file(path), path.string()} {}

archive::archive(std::vector<uint8_t> input, std::string label) : data{std::move(input)} {
  /// Decode VOLN's directory index and validate every uncompressed payload extent.
  binary_reader file{data, std::move(label)};
  if(file.integer<uint32_t>() != 0x4e4c4f56) file.fail("expected VOLN archive");
  file.take(6);
  auto const directory_size{file.integer<uint16_t>()};
  binary_reader names{file.take(directory_size), "VOLN directories"};
  std::vector<std::string> directories;
  while(names.remaining()) directories.push_back(normalise_path(names.cstring()));
  auto const count{file.integer<uint16_t>()};
  auto const table_size{file.integer<uint32_t>()};
  if(table_size != count * 18u) file.fail("invalid VOLN file table size");
  binary_reader table{file.take(table_size), "VOLN entries"};
  auto const payload_start{file.position()};
  std::vector<std::pair<size_t, size_t>> intervals;
  intervals.reserve(count);
  for(unsigned int index{}; index < count; ++index) {
    binary_reader filename{table.take(13), "VOLN filename"};
    auto const basename{filename.cstring()};
    auto const directory{table.integer<uint8_t>()};
    auto const offset{table.integer<uint32_t>()};
    if(directory >= directories.size() || offset < payload_start) table.fail("invalid VOLN entry");
    auto name{directories[directory]};
    if(!name.empty() && name.back() != '/') name += '/';
    name += normalise_path(basename);
    std::filesystem::path const relative{name};
    if(relative.is_absolute() || basename.empty() || basename.find_first_of("/\\") != std::string::npos) {
      table.fail("invalid resource path");
    }
    for(auto const &component : relative) {
      if(component == "..") table.fail("resource path escapes volume");
    }
    file.seek(offset);
    if(file.integer<uint8_t>() != 2) file.fail("unsupported VOLN compression codec");
    auto const size{file.integer<uint32_t>()};
    auto const stamp{file.integer<uint32_t>()};
    file.take(size);
    intervals.emplace_back(offset, file.position());
    archive_entry entry{
      .name{std::move(name)},
      .offset{offset + 9},
      .size{size},
      .timestamp{stamp},
    };
    auto const key{entry.name};
    if(!catalogue.emplace(key, std::move(entry)).second) file.fail("duplicate resource name");
  }
  std::ranges::sort(intervals);
  for(unsigned int index{1}; index < intervals.size(); ++index) {
    if(intervals[index - 1].second > intervals[index].first) file.fail("overlapping resource payloads");
  }
}

auto archive::entries() const->std::map<std::string, archive_entry, std::less<>> const & { return catalogue; }

auto archive::find(std::string_view name) const->archive_entry const * {
  auto const found{catalogue.find(normalise_path(name))};
  return found == catalogue.end() ? nullptr : &found->second;
}

auto archive::read(archive_entry const &entry) const->bytes {
  return bytes{data}.subspan(entry.offset, entry.size);
}

} // namespace earthsiege
