#pragma once
#include "resources/binary_reader.h"
#include <utility>

namespace earthsiege {

/// Explicit little-endian serialization; never writes a host C++ object layout.
class binary_writer {
  std::vector<uint8_t> output;
public:
  void append(bytes data) { output.insert(output.end(),data.begin(),data.end()); }
  template<std::integral T> void integer(T value) {
    using U=std::make_unsigned_t<T>;
    auto const bits{std::bit_cast<U>(value)};
    for(unsigned i{};i<sizeof(T);++i) output.push_back(static_cast<uint8_t>(bits>>(8*i)));
  }
  auto finish() &&->std::vector<uint8_t> { return std::move(output); }
};

} // namespace earthsiege
