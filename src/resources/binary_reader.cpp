#include "binary_reader.h"
#include <algorithm>
#include <fstream>
#include <limits>
#include <stdexcept>

namespace earthsiege {

auto read_file(std::filesystem::path const &path)->std::vector<uint8_t> {
  /// Read one original resource, keeping transport errors separate from decoding.
  std::ifstream file{path, std::ios::binary | std::ios::ate};
  if(!file) throw std::runtime_error{"cannot open " + path.string()};
  auto const length{file.tellg()};
  if(length < 0 || static_cast<uint64_t>(length) > std::numeric_limits<uint32_t>::max()) {
    throw std::runtime_error{"invalid resource size: " + path.string()};
  }
  std::vector<uint8_t> data(static_cast<size_t>(length));
  file.seekg(0);
  if(!data.empty() && !file.read(reinterpret_cast<char *>(data.data()), length)) {
    throw std::runtime_error{"cannot read " + path.string()};
  }
  return data;
}

auto normalise_path(std::string_view name)->std::string {
  /// Original resource names use DOS separators and case-insensitive matching.
  std::string result{name};
  for(auto &character : result) {
    if(character == '\\') character = '/';
    if(character >= 'A' && character <= 'Z') character += 'a' - 'A';
  }
  return result;
}

auto crc32(bytes data)->uint32_t {
  /// Produce the standard reflected CRC used by independent asset fixtures.
  uint32_t value{0xffffffff};
  for(auto const byte : data) {
    value ^= byte;
    for(unsigned int bit{}; bit < 8; ++bit) {
      value = (value >> 1) ^ ((value & 1) ? 0xedb88320u : 0u);
    }
  }
  return ~value;
}

binary_reader::binary_reader(bytes input, std::string label) :
  source{input},
  context{std::move(label)} {}

auto binary_reader::position() const->size_t { return cursor; }
auto binary_reader::remaining() const->size_t { return source.size() - cursor; }

void binary_reader::seek(size_t position) {
  if(position > source.size()) fail("seek outside resource");
  cursor = position;
}

auto binary_reader::take(size_t amount)->bytes {
  if(amount > remaining()) fail("truncated resource");
  auto const result{source.subspan(cursor, amount)};
  cursor += amount;
  return result;
}

auto binary_reader::cstring()->std::string {
  auto const tail{source.subspan(cursor)};
  auto const end{std::ranges::find(tail, uint8_t{})};
  if(end == tail.end()) fail("unterminated string");
  size_t const length{static_cast<size_t>(end - tail.begin())};
  auto const result{take(length)};
  take(1);
  return {reinterpret_cast<char const *>(result.data()), result.size()};
}

void binary_reader::require_end() const {
  if(remaining()) fail("unexpected trailing data");
}

void binary_reader::fail(std::string_view message) const {
  throw std::runtime_error{context + " at byte " + std::to_string(cursor) + ": " + std::string{message}};
}

} // namespace earthsiege
