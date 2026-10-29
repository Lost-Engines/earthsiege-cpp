#pragma once
#include "resources/binary_reader.h"
namespace earthsiege {
struct career_entry {std::string name;int16_t portrait{};std::string career_file,hangar_file;};
struct career_index {int16_t selected{-1};std::vector<career_entry> entries;};
career_index read_career_index(bytes source);
std::vector<uint8_t> write_career_index(career_index const &index);
}
