#pragma once

#include <array>
#include <cstdint>
#include <span>
#include <vector>
#include "simulation/random_state.h"

namespace earthsiege {

struct music_contact {
  int32_t distance{0};
  uint8_t side{0};
  bool disabled{false};
  bool dead{false};
  bool waiting{false};
  bool visible{false};
};

struct combat_music_state {
  int16_t track{-1};
  bool battle{false};
  uint32_t deadline{0};
  bool fading{false};
};

struct music_playback {
  std::array<uint8_t, 6> volume{100, 100, 100, 100, 100, 100};
  std::array<bool, 6> playing{};
};

enum class music_action : uint8_t {
  load,
  play,
  stop,
  volume,
};

struct music_request {
  music_action action;
  uint8_t track;
  uint8_t value{0};
};

struct music_contacts {
  std::vector<music_contact> actors;
  std::vector<music_contact> flyers;
};

std::vector<music_request> advance_combat_music(combat_music_state &state, music_playback &playback, std::span<music_contact const> actors,
                                                std::span<music_contact const> flyers, uint32_t clock, bool enabled, bool driver, random_state &random);

} // namespace earthsiege
