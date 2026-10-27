#include "random_state.h"
#include <algorithm>
#include <bit>
#include <limits>
#include <stdexcept>

namespace earthsiege {

random_state::random_state(int16_t advances) : current{
  .words{
    // original GO 0x81720, copied by 0x386c8 before the requested warm-up draws
    56611, 38195, 62413, 19394, 63458, 41695, 43632, 24746,
    5195, 35144, 6442, 27405, 40459, 64497, 34311, 18013,
    60148, 27133, 27716, 6951, 56177, 58255, 43956, 56391,
    4062, 19854, 64637, 36152, 22856, 58223, 43554, 12070,
    17402, 46501, 8427, 60805, 62915, 33183, 11676, 45677,
    59691, 38869, 15777, 35309, 58356, 29877, 44, 54111,
    15112, 3573, 62078, 49776, 9609, 11915, 25426, 55853,
  },
  .first{24},
  .second{55},
} {
  int const draws{advances == std::numeric_limits<int16_t>::min() ? 32768 : std::max(0, static_cast<int>(advances))};
  for(int index{}; index < draws; ++index) next();
}

random_state::random_state(state const &initial) : current{initial} {
  if(current.first >= word_count || current.second >= word_count) throw std::runtime_error{"invalid original random-state indices"};
}

auto random_state::next()->int16_t {
  /// GO 0x38723: two advancing taps and explicitly wrapping 16-bit addition.
  auto &word{current.words[current.second]};
  word = static_cast<uint16_t>(word + current.words[current.first]);
  auto const result{std::bit_cast<int16_t>(word)};
  current.first = (current.first + 1) % word_count;
  current.second = (current.second + 1) % word_count;
  return result;
}

auto random_state::bounded(int16_t limit)->int16_t {
  /// GO 0x38774 does not advance the generator when the requested limit is zero.
  if(!limit) return 0;
  int const value{std::bit_cast<uint16_t>(next()) & 0x7fff};
  return static_cast<int16_t>(value % limit);
}

auto random_state::snapshot() const->state const & { return current; }

} // namespace earthsiege
