#pragma once
#include "resources/binary_reader.h"

namespace earthsiege {

struct interface_string {
  std::string text;                                                        // original CP437 bytes, directly usable by DFN fonts
  std::vector<uint8_t> attributes;
};
using interface_strings = std::vector<std::vector<interface_string>>;
interface_strings read_interface_strings(bytes source);

} // namespace earthsiege
