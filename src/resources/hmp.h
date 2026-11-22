#pragma once
#include <array>
#include "resources/binary_reader.h"
namespace earthsiege {
struct hmp_branch {
  uint32_t event_offset{0};
  uint8_t id{0};
  uint8_t program{0};
  uint8_t loops{0};
  std::vector<uint8_t> controls;
};
struct hmp_track {
  uint32_t offset{0};
  uint32_t length{0};
  uint32_t channel{0};
  std::array<uint32_t, 5> devices{};
  std::vector<hmp_branch> branches;
};
struct hmp_song {
  std::vector<uint8_t> data;
  uint32_t tick_rate{0};
  std::array<uint32_t, 16> channel_priorities{};
  std::vector<hmp_track> tracks;
};
uint32_t read_hmp_delta(binary_reader &file);
hmp_song read_hmp(bytes data);
} // namespace earthsiege
