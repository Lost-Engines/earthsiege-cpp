#pragma once
#include <cstdint>
#include <span>
#include <vector>
#include "resources/model.h"

namespace earthsiege {

struct model_detail {
  unsigned int projection_shift{8};
  int16_t scale{1024};
  unsigned int bias{};
};

auto bsp_draw_order(bsp_part const &part, vec3<int32_t> const &eye, std::span<model_transform const> transforms)->std::vector<uint16_t>;
auto cell_draw_index(cell_part const &part, std::span<uint16_t const> frames)->unsigned int;
/// Select an actor's top-level model; its descending thresholds differ from nested detail parts
auto actor_detail_index(size_t model_count, std::span<int32_t const> thresholds, int16_t radius, vec3<int32_t> const &eye, model_detail const &detail)->unsigned int;
auto detail_draw_index(detail_part const &part, int16_t radius, vec3<int32_t> const &eye, model_detail const &detail)->unsigned int;

} // namespace earthsiege
