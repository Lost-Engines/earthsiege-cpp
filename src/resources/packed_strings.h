#pragma once
#include "resources/binary_reader.h"
namespace earthsiege {
/// GO's offset table and CP437 string pool, shared by dialog and shell text.
std::vector<std::string> read_packed_strings(bytes source);
}
