#pragma once
#include <string>
#include <vector>
#include "resources/binary_reader.h"

namespace earthsiege {

std::vector<std::string> read_resource_names(bytes source);

} // namespace earthsiege
