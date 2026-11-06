#pragma once
#include <cstddef>
#include <cstdint>
namespace earthsiege {
enum class game_edition { original, expansion };
constexpr size_t weapon_count(game_edition edition) { return edition == game_edition::expansion ? 22U : 17U; }
constexpr size_t human_herc_count(game_edition edition) { return edition == game_edition::expansion ? 9U : 8U; }
constexpr size_t human_herc_index(int16_t chassis) { return chassis == 16 ? 8U : static_cast<size_t>(chassis); }
constexpr int16_t human_herc_id(size_t index) { return index == 8 ? 16 : static_cast<int16_t>(index); }
} // namespace earthsiege
