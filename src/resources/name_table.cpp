#include "name_table.h"

namespace earthsiege {

std::vector<std::string> read_resource_names(bytes source) {
  while(!source.empty() && (source.back() == '\n' || source.back() == '\r')) source = source.first(source.size() - 1);
  binary_reader file{source, "resource names"};
  std::vector<std::string> result;
  while(file.remaining()) result.push_back(file.cstring());
  return result;
}

} // namespace earthsiege
