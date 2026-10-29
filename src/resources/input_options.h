#pragma once
#include <array>
#include <cstdint>
#include <string_view>
namespace earthsiege {
struct keyjoy_options {
 bool tilt_reverse{},backturn_reverse{},missile_reverse{},invalid_token{};
};
/// Original whitespace-token syntax, including comments and DOS text EOF.
keyjoy_options read_keyjoy(std::string_view text);
/// Original requested transition, -1 when none applies to the current view.
int cockpit_hat_request(unsigned view,std::array<uint8_t,4> const &hat);
}
