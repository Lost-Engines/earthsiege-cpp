#pragma once
#include <span>
#include "resources/model.h"

namespace earthsiege {

std::span<model_part> model_children(model_part &part);
std::span<model_part const> model_children(model_part const &part);
model_part const *find_model_part(model_part const &root, int16_t id);
model_part clone_model_part(model_part const &source);

} // namespace earthsiege
