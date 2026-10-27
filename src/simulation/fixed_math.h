#pragma once
#include <cstdint>
#include "vectorstorm/vector/vector2.h"
#include "vectorstorm/vector/vector3.h"

namespace earthsiege {

int16_t wrap_word(int32_t value);
int32_t wrap_long(int64_t value);
int32_t fast_distance_2d(vec2<int32_t> const &delta);
int32_t fast_distance(vec3<int32_t> const &delta);

} // namespace earthsiege
