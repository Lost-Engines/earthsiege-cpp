#pragma once
#include <functional>
#include <optional>
#include "simulation/random_state.h"
#include "simulation/weapon_bank.h"

namespace earthsiege {

struct herc_fire_state {
  std::optional<size_t> retained_weapon;                                   // ELF keeps firing without a fresh ranking while ready/in range
  bool skip_selection{false};
};
struct herc_fire_target {
  vec3<int32_t> point;
  uint16_t heading{0};
  int16_t relative_facing{0};
};
struct herc_accuracy {
  bool enemy{false};
  bool campaign{false};
  int16_t difficulty{0};
};
struct herc_fire_callbacks {
  std::function<std::optional<size_t>(int32_t, int16_t)> select_weapon;
  std::function<int16_t()> target_speed;
  std::function<vec3<int16_t>(vec3<int32_t> const &)> aim;
  std::function<void(size_t)> fire;
};

void aim_and_fire_herc(herc_fire_state &state, weapon_bank const &bank, weapon_catalogue const &definitions, vec3<int32_t> const &position, herc_fire_target const &target, herc_accuracy const &accuracy, random_state &random, herc_fire_callbacks const &callbacks);

} // namespace earthsiege
