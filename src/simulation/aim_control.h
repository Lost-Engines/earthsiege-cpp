#pragma once
#include <cstdint>
#include "resources/herc_profile.h"

namespace earthsiege {

struct aiming_state {
  int16_t rate{0};
  int16_t angle{0};
};

void update_aim(aiming_state &state, aiming_profile const &profile, int16_t input, int16_t time_ratio);

} // namespace earthsiege
