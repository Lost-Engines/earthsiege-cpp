#pragma once

#include <algorithm>
#include <cstdint>
#include <deque>
#include <mutex>
#include <span>

namespace earthsiege {

/// Thread-safe transport only: all mixing and synthesis happen in the game.
class pcm_queue {
  std::mutex lock;
  std::deque<int16_t> samples;
  bool paused{};

public:
  void queue(std::span<int16_t const> input) {
    std::lock_guard guard{lock};
    samples.insert(samples.end(), input.begin(), input.end());
  }
  void read(std::span<int16_t> output) {
    std::lock_guard guard{lock};
    std::ranges::fill(output, int16_t{});
    if(paused) return;
    size_t const count{std::min(output.size(), samples.size())};
    std::copy_n(samples.begin(), count, output.begin());
    samples.erase(samples.begin(), samples.begin() + static_cast<std::ptrdiff_t>(count));
  }
  void pause(bool value) {
    std::lock_guard guard{lock};
    paused = value;
  }
  void clear() {
    std::lock_guard guard{lock};
    samples.clear();
  }
};

} // namespace earthsiege
