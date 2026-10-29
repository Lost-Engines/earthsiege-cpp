#include "packed_strings.h"
namespace earthsiege {
std::vector<std::string> read_packed_strings(bytes source) {
  binary_reader file{source,"packed strings"};
  auto const count{file.integer<uint32_t>()},amount{file.integer<uint32_t>()};
  if(count>32767)file.fail("excessive string count");
  std::vector<uint16_t> offsets;
  for(unsigned i{};i<count;++i)offsets.push_back(file.integer<uint16_t>());
  auto const pool{file.take(amount)};file.require_end();
  std::vector<std::string> result;
  for(auto offset:offsets){binary_reader text{pool,"packed string"};text.seek(offset);result.push_back(text.cstring());}
  return result;
}
}
