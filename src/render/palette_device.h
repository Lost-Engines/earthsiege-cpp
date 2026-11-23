#pragma once
#include <optional>
#include <utility>
#include "resources/artwork.h"
namespace earthsiege {
/// DBSIM 27f74 / 88684: CPU palette and displayed VGA DAC are distinct.
class palette_device {
  palette displayed;
  std::array<std::array<uint8_t, 3>, 256> registers{};
  palette const *source{nullptr};
  std::optional<std::pair<unsigned, unsigned>> dirty;
  unsigned next{0};
  void upload(palette const &, unsigned first, unsigned count);
public:
  void changed(unsigned first, unsigned count);
  void present(palette const &);
  palette const &colours() const {
    return displayed;
  }
  auto const &dac_registers() const {
    return registers;
  }
  unsigned next_index() const {
    return next;
  }
};
} // namespace earthsiege
