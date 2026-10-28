#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include "resources/binary_reader.h"

namespace earthsiege {

struct presentation_instruction {
  uint16_t opcode;
  std::vector<int16_t> arguments;
  std::string text;
  std::vector<int16_t> points;
};
struct presentation_label { uint16_t number; std::string name; };
struct presentation_program {
  std::vector<std::vector<presentation_instruction>> pages;
  std::vector<presentation_label> labels;
  std::vector<uint8_t> bytecode;
  uint8_t label_page(int16_t number) const;
};

/// Dynamix TTM 4.14 chunk container and bytecode, retaining every operand.
presentation_program read_presentation(bytes source);

} // namespace earthsiege
