#include "frame_time.h"
#include <algorithm>
#include <stdexcept>
#include "simulation/fixed_math.h"

namespace earthsiege {

frame_ratio ratio_from_period(int16_t const elapsed, int16_t const nominal) {
  if(nominal == 0) throw std::domain_error{"zero nominal frame period"};
  /// The original clamps the low signed word, not the full quotient
  return std::clamp<int16_t>(wrap_word(static_cast<int32_t>(elapsed) * frame_ratio_one / nominal), 64, 450);
}

int16_t scale_frame_time(int16_t const value, frame_ratio const ratio) {
  return std::max<int16_t>(-32767, wrap_word((static_cast<int32_t>(value) * ratio + 128) >> 8));
}

} // namespace earthsiege
