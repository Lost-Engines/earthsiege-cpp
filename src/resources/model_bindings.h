#pragma once
#include <cstdint>
#include <span>
#include "resources/model.h"

namespace earthsiege {

/// Bind lower-detail HERC geometry to the full-detail articulated instance
void bind_herc_detail_nodes(model_resource &model, std::span<uint8_t const> part_ids);

} // namespace earthsiege
