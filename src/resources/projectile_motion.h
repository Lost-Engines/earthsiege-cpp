#pragma once
#include <vector>
#include "resources/binary_reader.h"
#include "resources/edition.h"

namespace earthsiege {

struct bullet_motion {
  int16_t model{0};
  int16_t lifetime{0};
  int16_t collision_radius{0};
  int16_t animation_period{0};
  int16_t shot_noise{0};
  int16_t spread{0};
  int16_t damage_falloff{0};
};

struct rocket_motion {
  int16_t model{0};
  int16_t lifetime{0};
  int16_t acceleration{0};
  int16_t collision_radius{0};
  int16_t animation_period{0};
  int16_t cell_sequence{0};
  int16_t shot_noise{0};
};

struct projectile_motions {
  std::vector<bullet_motion> bullets;
  std::vector<rocket_motion> rockets;
};

std::vector<bullet_motion> read_bullet_motions(bytes source, game_edition edition = game_edition::original);
std::vector<rocket_motion> read_rocket_motions(bytes source);

} // namespace earthsiege
