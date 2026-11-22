#pragma once
#include "resources/binary_reader.h"

namespace earthsiege {
struct portrait_frame {
  uint8_t frame{0};
  uint8_t duration{0};
};
std::vector<portrait_frame> read_portrait_sync(bytes source);
/// Simulation SNC cursor driven by the original nominal 60 Hz mouth callback.
class portrait_animation {
  std::vector<portrait_frame> frames;
  size_t cursor{0};
  uint32_t clock{0};
  uint32_t deadline{0};
  int16_t current{-1};
  void next();
public:
  explicit portrait_animation(std::vector<portrait_frame> timeline);
  void start(uint32_t ticks = 0);
  void advance(uint32_t ticks);
  int16_t frame() const;
  uint32_t next_tick() const;
};
} // namespace earthsiege
