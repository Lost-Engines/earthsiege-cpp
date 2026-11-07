#pragma once
#include <optional>
#include <cstdint>
#include "simulation/keyboard_input.h"
namespace earthsiege {
// GLFW keys use the original PC set-1 command numbers. Shift changes text,
// but is discarded by the simulator's command filter; Alt and Ctrl are retained.
std::optional<uint16_t> keyboard_command(int key, bool alt, bool control);
std::optional<held_key> keyboard_held_key(int key);
}
