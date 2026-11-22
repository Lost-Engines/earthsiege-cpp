#pragma once
#include <array>
#include <cstdint>
#include <utility>
#include <vector>
#include "vectorstorm/vector/vector2.h"
#include "resources/binary_reader.h"
#include "resources/edition.h"

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
  uint16_t kind{0};
  uint32_t timestamp{0};
};

struct tape_frame {
  uint16_t command{0};
  vec2<int16_t> primary_axes;
  vec2<int16_t> secondary_axes;
  uint16_t delta{0};
  uint16_t buttons{0};
  std::vector<tape_mouse_event> mouse;
  uint16_t extended_buttons{0};
  std::vector<uint16_t> commands;
};

struct input_tape {
  std::array<std::vector<uint8_t>, std::to_underlying(tape_file::count)> starting_files;
  std::vector<tape_frame> frames;
  size_t stream_offset{0};
  game_edition edition{game_edition::original};
};

auto read_input_tape(bytes source, game_edition edition = game_edition::original)->input_tape;

} // namespace earthsiege
