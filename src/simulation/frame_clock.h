#pragma once
#include <cstdint>
#include <optional>
#include "frame_time.h"
namespace earthsiege {
struct frame_clock_state {
  int16_t maximum{120};
  int16_t nominal{12};
  int16_t alternate_nominal{12};
  int16_t countdown{120};
  int16_t elapsed{12};
  int16_t ratio{256};
  int16_t alternate_ratio{256};
  uint32_t fast{0};
  uint32_t slow{0};
};
/// Original 0x63e3c interrupt and 0x63f40 live pacing, with no host busy-wait.
class frame_clock {
  frame_clock_state state_;
public:
  explicit frame_clock(frame_clock_state initial = {}) : state_{initial} {}
  void advance(unsigned ticks);
  bool ready() const;
  std::optional<frame_ratio> take();
  frame_clock_state const &state() const {
    return state_;
  }
};
}
