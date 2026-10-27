#pragma once
#include <array>
#include <cstdint>
#include <utility>
#include <vector>
#include "vectorstorm/vector/vector2.h"
#include "resources/binary_reader.h"

namespace earthsiege {

enum class tape_file : unsigned int {
  script,
  player,
  campaign_flags,
  preferences,
  restore,
  objective_text,
  keyjoy,
  count,
};

struct tape_mouse_event {
  vec2<int32_t> position;
  uint16_t kind{};
  uint32_t timestamp{};
};

struct tape_frame {
  uint16_t command{};
  vec2<int16_t> primary_axes;
  vec2<int16_t> secondary_axes;
  uint16_t delta{};
  uint16_t buttons{};
  std::vector<tape_mouse_event> mouse;
};

struct input_tape {
  std::array<std::vector<uint8_t>, std::to_underlying(tape_file::count)> starting_files;
  std::vector<tape_frame> frames;
  size_t stream_offset{};
};

auto read_input_tape(bytes source)->input_tape;

} // namespace earthsiege
