#pragma once
#include "render/model_traversal.h"
namespace earthsiege {
/// DBSIM 57c08: whole-model scale/bias and independent nested-part biases.
struct shape_preferences {
  model_detail actor;
  unsigned ordinary{0};
  unsigned weapon{0};
  unsigned herc{0};
};
shape_preferences shape_detail(uint8_t value, bool reduced_memory = false);
}
