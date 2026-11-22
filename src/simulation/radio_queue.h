#pragma once
#include <array>
#include <cstdint>
#include <optional>
#include "simulation/fixed_math.h"

namespace earthsiege {
struct radio_event {
  uint16_t id{0};
  std::optional<size_t> speaker;
  vec3<int32_t> position{};
  std::optional<size_t> subject;
  bool cancel{false};
};
struct radio_descriptor {
  uint8_t id{0};
  uint8_t flags{0};
  uint8_t priority{0};
  uint8_t minimum{0};
  uint8_t maximum{0};
  uint8_t earliest{0};
  uint8_t expiry{0};
};
struct radio_message {
  radio_event event;
  uint8_t flags{0};
  uint32_t text{0};
  uint8_t priority{0};
  uint32_t minimum{0};
  uint32_t maximum{0};
  uint32_t earliest{0};
  uint32_t expiry{0};
};
struct radio_queue_state {
  std::array<radio_message, 10> slots{};
  std::array<uint8_t, 10> order{};
  std::array<bool, 10> allocated{};
  size_t count{0};
};
/// Original ten-slot priority queue, including the short insertion shift and
/// double-indexed full-queue eviction. Untouched index words start at zero.
class radio_queue {
  radio_queue_state data;
public:
  bool push(radio_descriptor const &descriptor, uint32_t text, radio_event event, uint32_t ticks);
  void erase(size_t index);
  void expire(uint32_t ticks);
  radio_message *front();
  radio_message &slot(size_t index);
  radio_queue_state const &state() const;
};
} // namespace earthsiege
