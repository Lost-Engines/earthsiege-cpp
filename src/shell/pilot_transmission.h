#pragma once
#include "resources/portrait_sync.h"
#include "simulation/random_state.h"

namespace earthsiege {
struct pilot_transmission_state {
  uint32_t phase{},previous{},deadline{};
  bool permanent_static{};
  uint8_t death_frame{},previous_death_frame{};
};
struct pilot_transmission_update {
  bool ready{},cancel{};
  // Original painter boundaries: 0 idle, 1 static, 2 mouth, 3 sound request.
  std::vector<std::array<int32_t,2>> calls;
};
class pilot_transmission {
  uint8_t message;
  portrait_animation animation;
  bool started{};
  pilot_transmission_state data;
public:
  pilot_transmission(uint8_t message_id,std::vector<portrait_frame> timeline,uint32_t ticks);
  void begin(uint8_t message_id,std::vector<portrait_frame> timeline,uint32_t ticks);
  pilot_transmission_update advance(uint32_t ticks,bool shown,bool dead,random_state &random);
  pilot_transmission_state const &state()const;
  int16_t mouth()const;
};
} // namespace earthsiege
