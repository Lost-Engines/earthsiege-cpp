#include "interface_strings.h"

namespace earthsiege {

interface_strings read_interface_strings(bytes source) {
  binary_reader file{source, "interface strings"};
  if(file.integer<uint32_t>() != file.remaining()) file.fail("string resource size differs");
  interface_strings result;
  while(file.remaining()) {
    auto const count{file.integer<uint16_t>()};
    auto &group{result.emplace_back()};
    for(unsigned int i{}; i < count; ++i) {
      auto const text{file.take(file.integer<uint16_t>())};
      if(text.empty() || text.back()) file.fail("interface string lacks final NUL");
      auto const attributes{file.take(file.integer<uint8_t>())};
      group.push_back({{reinterpret_cast<char const *>(text.data()), text.size() - 1}, {attributes.begin(), attributes.end()}});
    }
  }
  return result;
}

} // namespace earthsiege
