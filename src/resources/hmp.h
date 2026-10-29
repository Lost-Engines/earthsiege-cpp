#pragma once
#include <array>
#include "resources/binary_reader.h"
namespace earthsiege {
struct hmp_branch {
  uint32_t event_offset{};
  uint8_t id{}, program{}, loops{};
  std::vector<uint8_t> controls;
};
struct hmp_track {
  uint32_t offset{}, length{}, channel{};
  std::array<uint32_t,5> devices{};
  std::vector<hmp_branch> branches;
};
struct hmp_song {
  std::vector<uint8_t> data;
  uint32_t tick_rate{};
  std::array<uint32_t,16> channel_priorities{};
  std::vector<hmp_track> tracks;
};
uint32_t read_hmp_delta(binary_reader &file);
hmp_song read_hmp(bytes data);
} // namespace earthsiege
