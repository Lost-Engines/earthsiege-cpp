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
  int16_t flag{};
  flag_comparison comparison{};
  int16_t expected{};
};

struct random_draw {
  int16_t limit{};
  int16_t source_result{};
  int16_t reserved{};
};

struct random_interval {
  uint16_t first{};
  uint16_t last{};
  int16_t reserved{};
};

struct random_group {
  int16_t group{};
  int16_t reserved{};
  int16_t limit{};
};

using condition_rule = std::variant<flag_test, random_draw, random_interval, random_group>;

struct mission_condition {
  int16_t id{};
  int16_t parent{};
  condition_rule rule;
  int16_t scratch{};
  std::optional<int16_t> result;
};

auto read_conditions(binary_reader &file)->std::vector<mission_condition>;
auto evaluate_conditions(std::span<mission_condition const> source, std::span<int16_t const> flags, random_state &random)->std::vector<mission_condition>;
auto condition_enabled(std::span<mission_condition const> active, int16_t id)->bool;

} // namespace earthsiege
