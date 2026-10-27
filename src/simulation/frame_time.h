#pragma once
#include <cstdint>

namespace earthsiege {

using frame_ratio = int16_t;
inline frame_ratio constexpr frame_ratio_one{256};
inline int16_t constexpr nominal_frame_milliseconds{100};

frame_ratio ratio_from_period(int16_t elapsed, int16_t nominal);
int16_t scale_frame_time(int16_t value, frame_ratio ratio);

} // namespace earthsiege
