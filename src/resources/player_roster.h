#pragma once
#include <array>
#include <cstdint>
#include <vector>
#include "resources/binary_reader.h"

namespace earthsiege {

/// Percentage condition stored by the shell, independently of live damage arrays.
struct herc_condition {
  std::array<int16_t, 13> armour{};
  std::array<int16_t, 10> internal{};
  std::array<int16_t, 10> equipment{};
};

struct player_herc {
  int16_t slot{0};
  int16_t field{0};
  int16_t chassis{0};
  std::vector<int16_t> weapons;
  std::vector<int16_t> weapon_values;
  int16_t extra{0};
  herc_condition condition;
};

struct player_roster {
  int16_t selected{0};
  std::vector<player_herc> hercs;
  std::array<uint8_t, 17> flags{};
};

player_roster read_player_roster(bytes source);

} // namespace earthsiege
