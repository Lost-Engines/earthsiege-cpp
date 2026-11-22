#pragma once
#include <cstdint>
#include <optional>
#include <span>
#include <variant>
#include <vector>
#include "resources/binary_reader.h"

namespace earthsiege {

class random_state;

enum class flag_comparison : int16_t {
  equal = 0x114,
  not_equal,
  less,
  greater,
};

struct flag_test {
  int16_t flag{0};
  flag_comparison comparison{};
  int16_t expected{0};
};

struct random_draw {
  int16_t limit{0};
  int16_t source_result{0};
  int16_t reserved{0};
};

struct random_interval {
  uint16_t first{0};
  uint16_t last{0};
  int16_t reserved{0};
};

struct random_group {
  int16_t group{0};
  int16_t reserved{0};
  int16_t limit{0};
};

using condition_rule = std::variant<flag_test, random_draw, random_interval, random_group>;

struct mission_condition {
  int16_t id{0};
  int16_t parent{0};
  condition_rule rule;
  int16_t scratch{0};
  std::optional<int16_t> result;
};

auto read_conditions(binary_reader &file)->std::vector<mission_condition>;
auto evaluate_conditions(std::span<mission_condition const> source, std::span<int16_t const> flags, random_state &random)->std::vector<mission_condition>;
auto condition_enabled(std::span<mission_condition const> active, int16_t id)->bool;

} // namespace earthsiege
