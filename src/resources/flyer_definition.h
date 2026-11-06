#pragma once
#include <array>
#include <string_view>
#include "resources/artwork.h"
#include "resources/combat.h"
#include "resources/model.h"

namespace earthsiege {
class resource_store;

struct flyer_profile {
  game_edition edition{game_edition::original};
  int16_t pitch_step{0};
  int16_t bank_rate{0};
  int16_t bank_acceleration{0};
  int16_t bank_turn_shift{0};
  int16_t maximum_bank{0};
  int16_t forward_step{0};                                                 // local Y displacement per movement dispatch, not per elapsed millisecond
  std::vector<int16_t> source = std::vector<int16_t>(17);                                        // retain fields whose steering consumers are not yet ported
};
struct flyer_definition {
  flyer_profile profile;
  model_resource models;
  std::vector<bitmap> textures;
  damage_definition damage;
  std::vector<collision_part> collision;
};
flyer_definition read_flyer_definition(resource_store const &resources, std::string_view name);
} // namespace earthsiege
