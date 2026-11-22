#pragma once
#include <cstdint>
#include <span>
#include <string>
#include <variant>
#include <vector>
#include "resources/binary_reader.h"

namespace earthsiege {

enum class mission_text_kind : uint32_t {
  briefing = 0x3a465242,
  objectives = 0x3a4a424f,
  debriefing = 0x3a424544,
};

struct conditional_phrase {
  int16_t first{0};
  int16_t last{0};
  std::string text;
};

struct counter_phrase {
  unsigned int counter{0};
  std::vector<conditional_phrase> choices;
};

using text_segment = std::variant<std::string, counter_phrase>;

struct text_program {
  unsigned int maximum_text{0};
  std::vector<text_segment> segments;
};

struct objective_text {
  uint8_t marker{0};
  std::string text;
};

using mission_text_content = std::variant<text_program, std::vector<objective_text>>;

struct mission_text_section {
  mission_text_kind kind{};
  mission_text_content content;
};

struct mission_text {
  std::vector<mission_text_section> sections;
  auto find(mission_text_kind kind, unsigned int index = 0) const->mission_text_section const *;
};

auto read_mission_text(bytes source)->mission_text;
auto render_text(text_program const &program, std::span<int16_t const> counters)->std::string;

} // namespace earthsiege
