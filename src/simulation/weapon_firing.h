#pragma once
#include "simulation/weapon_bank.h"

namespace earthsiege {

enum class shot_kind : uint8_t {
  bullet,
  rocket,
  scaled,
  beam,
};

struct weapon_muzzle {
  model_transform transform;
  vec3<int32_t> position;
  vec3<uint16_t> angles;
};

struct weapon_shot {
  shot_kind kind{shot_kind::bullet};
  size_t projectile{0};
  int16_t charge{-1};
  int32_t range{0};
  weapon_muzzle muzzle;
  int16_t inherited_speed{0};                                              // skimmer rockets add local forward speed to the 500-unit launch speed
};

weapon_muzzle prepare_weapon_muzzle(weapon_mount const &mount, weapon_definition const &definition, model_part const &root, std::span<model_transform const> transforms, model_transform const &placement, vec2<int16_t> const &aim);
/// Caller checks readiness and trigger policy; this performs the original discharge and spawn requests
std::vector<weapon_shot> discharge_weapon(weapon_state &state, weapon_catalogue const &definitions, weapon_mount const &mount, weapon_muzzle const &muzzle, bool free_ammunition);

} // namespace earthsiege
