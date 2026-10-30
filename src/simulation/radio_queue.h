#pragma once
#include "simulation/fixed_math.h"
#include <array>
#include <cstdint>
#include <optional>

namespace earthsiege {
struct radio_event {
  uint16_t id{};
  std::optional<size_t> speaker;
  vec3<int32_t> position{};
  std::optional<size_t> subject;
  bool cancel{};
};
struct radio_descriptor {
  uint8_t id{}, flags{}, priority{}, minimum{}, maximum{}, earliest{}, expiry{};
};
struct radio_message {
  radio_event event;
  uint8_t flags{};
  uint32_t text{};
  uint8_t priority{};
  uint32_t minimum{}, maximum{}, earliest{}, expiry{};
};
struct radio_queue_state {
  std::array<radio_message,10> slots{};
  std::array<uint8_t,10> order{};
  std::array<bool,10> allocated{};
  size_t count{};
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
