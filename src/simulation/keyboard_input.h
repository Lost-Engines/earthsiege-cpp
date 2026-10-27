#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include "vectorstorm/vector/vector2.h"

namespace earthsiege {

enum class held_key : size_t {
  forward_left, forward, forward_right, left, right,
  reverse_left, reverse, reverse_right,
  aim_down, aim_left, aim_right, aim_up,
  fire, enter, count,
};

inline size_t constexpr held_key_count{static_cast<size_t>(held_key::count)};

struct keyboard_latches {
  std::array<uint8_t, held_key_count> held{};
  std::array<uint8_t, held_key_count> released{};
  std::array<uint8_t, held_key_count> consumed{};
  bool alt{false};
};

struct keyboard_sample {
  vec2<int32_t> movement;
  vec2<int32_t> aiming;
  bool fire{false};
  bool enter{false};
};

/// Retain short key taps until one simulation update consumes them
class keyboard_input {
  keyboard_latches current;

public:
  explicit keyboard_input(keyboard_latches initial = {});
  void event(held_key key, bool pressed, bool alt);
  keyboard_sample sample(bool alternate_movement = false);
  void clear();
  keyboard_latches const &state() const;
};

} // namespace earthsiege
