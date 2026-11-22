#pragma once
#include <string>
#include "artwork.h"
namespace earthsiege {
// The CD player's word-copy delta stream. A guard word bounds two malformed
// writes in the localized original intro without exposing it as image data.
class dfl_movie {
  std::vector<uint8_t> source;
  size_t position{128};
  std::array<uint8_t, 64002> raster{};
  std::array<uint8_t, 768> base_palette{};
  std::array<uint8_t, 768> display_palette{};
  palette colours_;
  int fade_length{0};
  int fade_remaining{0};
  int fade_direction{0};
  std::array<int, 3> fade_colour{};
  std::vector<std::string> samples;
  bool done{false};
  void delta(bytes);
  void palette_packets(bytes);
  void fade_step();
  void fade(int amount);
  void update_palette();
public:
  explicit dfl_movie(bytes);
  bool advance();
  bool finished() const {
    return done;
  }
  bytes pixels() const {
    return {raster.data(), 64000};
  }
  palette const &colours() const {
    return colours_;
  }
  std::vector<std::string> take_samples();
};
} // namespace earthsiege
