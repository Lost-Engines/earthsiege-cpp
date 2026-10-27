#pragma once
#include <array>
#include <cstdint>

namespace earthsiege {

class random_state {
public:
  static unsigned int constexpr word_count{56};
  struct state {
    std::array<uint16_t, word_count> words{};
    unsigned int first{};
    unsigned int second{};
  };

private:
  state current;

public:
  explicit random_state(int16_t advances = 0);
  explicit random_state(state const &initial);
  auto next()->int16_t;
  auto bounded(int16_t limit)->int16_t;
  auto snapshot() const->state const &;
};

} // namespace earthsiege
