#pragma once
#include <array>
#include <cstdint>
#include <string_view>
#include "resources/edition.h"
namespace earthsiege {
struct keyjoy_options {
  bool tilt_reverse{false};
  bool backturn_reverse{false};
  bool missile_reverse{false};
  bool invalid_token{false};
  bool pedals_reverse{false};
};
/// Original whitespace-token syntax, including comments and DOS text EOF.
keyjoy_options read_keyjoy(std::string_view text, game_edition edition = game_edition::original);
/// Original requested transition, -1 when none applies to the current view.
int cockpit_hat_request(unsigned view, std::array<uint8_t, 4> const &hat);
}
