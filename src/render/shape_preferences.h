#pragma once
#include "render/model_traversal.h"
namespace earthsiege {
/// DBSIM 57c08: whole-model scale/bias and independent nested-part biases.
struct shape_preferences {
 model_detail actor;
 unsigned ordinary{},weapon{},herc{};
};
shape_preferences shape_detail(uint8_t value,bool reduced_memory=false);
}
