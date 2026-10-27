#pragma once
#include <bit>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace earthsiege {

using bytes = std::span<uint8_t const>;

auto read_file(std::filesystem::path const &path)->std::vector<uint8_t>;
auto normalise_path(std::string_view name)->std::string;
auto crc32(bytes data)->uint32_t;

class binary_reader {
  bytes source;
  std::string context;
  size_t cursor{};

public:
  explicit binary_reader(bytes source, std::string context = {});
  auto position() const->size_t;
  auto remaining() const->size_t;
  void seek(size_t position);
  auto take(size_t amount)->bytes;
  auto cstring()->std::string;
  void require_end() const;
  [[noreturn]] void fail(std::string_view message) const;

  template<std::integral T>
  auto integer()->T {
    /// Decode a little-endian value without unaligned loads or signed shifts.
    using U = std::make_unsigned_t<T>;
    auto const data{take(sizeof(T))};
    uint64_t value{};
    for(unsigned int index{}; index < sizeof(T); ++index) {
      value |= static_cast<uint64_t>(data[index]) << (index * 8);
    }
    return std::bit_cast<T>(static_cast<U>(value));
  }
};

} // namespace earthsiege
