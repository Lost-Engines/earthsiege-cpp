#pragma once
#include "resources/resource_store.h"
#include "vectorstorm/vector/vector2.h"
#include <array>
namespace earthsiege {
struct shell_herc {
 int16_t chassis{};
 std::vector<int16_t> painter_order;
 std::array<std::string,5> artwork;
 vec2<int32_t> origin;
 std::vector<vec2<int32_t>> construction_positions;
 std::string loadout_diagram,damage_diagram;
};
std::vector<shell_herc> read_shell_hercs(resource_store const &);
}
