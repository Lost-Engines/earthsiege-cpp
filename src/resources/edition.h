#pragma once
#include <cstddef>
namespace earthsiege {
enum class game_edition { original, expansion };
constexpr size_t weapon_count(game_edition edition) { return edition == game_edition::expansion ? 22U : 17U; }
constexpr size_t human_herc_count(game_edition edition) { return edition == game_edition::expansion ? 9U : 8U; }
} // namespace earthsiege
