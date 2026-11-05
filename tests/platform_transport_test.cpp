#include "platform/viewport.h"
#include "platform/pcm_queue.h"
#include <array>
#include <iostream>
#include <stdexcept>

namespace {
void require(bool value, char const *message) {
  if(!value) throw std::runtime_error{message};
}
}
int main() try {
  using earthsiege::viewport;
  auto const area{viewport::fit(1920, 1080)};
  require(area.width == 1600 && area.height == 1000 && area.x == 160 && area.y == 40, "integer square pixels");
  for(auto const size : {std::array{960, 540, 1920, 1080}, std::array{803, 601, 803, 601}, std::array{120, 100, 120, 100}}) {
    auto const view{viewport::fit(size[2], size[3])};
    for(auto const pixel : {std::array{0, 0}, std::array{319, 199}, std::array{160, 100}}) {
      auto const point{view.cursor(pixel[0], pixel[1], size[0], size[1], size[2], size[3])};
      require(view.logical(point[0], point[1], size[0], size[1], size[2], size[3]) == pixel, "HiDPI/letterbox cursor round trip");
    }
  }
  require(area.logical(0, 0, 1920, 1080, 1920, 1080)[0] < 0, "letterbox clicks stay outside game");
  earthsiege::pcm_queue queue;
  std::array<int16_t, 4> const first{-32768, 32767, 2, -2};
  std::array<int16_t, 2> const second{3, -3};
  std::array<int16_t, 8> output{};
  queue.queue(first);
  queue.pause(true);
  queue.read(output);
  require(output == std::array<int16_t, 8>{}, "paused output is silence");
  queue.queue(second);
  queue.pause(false);
  queue.read(output);
  require(output == std::array<int16_t, 8>{-32768, 32767, 2, -2, 3, -3, 0, 0}, "PCM ordering, signed range and underrun silence");
  queue.queue(first);
  queue.clear();
  queue.read(output);
  require(output == std::array<int16_t, 8>{}, "clear removes previous scene's audio");
  std::cout << "Viewport and PCM transport checks pass\n";
} catch(std::exception const &error) {
  std::cerr << error.what() << '\n';
  return 1;
}
