#pragma once
#include <array>
#include <vector>
#include "resources/binary_reader.h"

namespace earthsiege {

enum class effect_light_kind : int16_t { none, ordinary, distance_gated };

struct explosion_definition {
  int16_t model{0};
  int16_t cell_sequence{0};
  int16_t texture_bank{0};
  int16_t animation_period{0};
  bool secondary{false};
  effect_light_kind light{effect_light_kind::none};
  std::array<int16_t, 12> light_curve{};
  int32_t light_range{0};
  int16_t sound{0};
  bool alternate_object_kind{false};
  uint8_t frame_light(uint16_t frame) const;
};

struct weighted_debris_model {
  int16_t model{0};
  int16_t weight{0};
};

struct debris_definition {
  int16_t count_limit{0};
  std::vector<weighted_debris_model> models;
  int16_t total_weight{0};
};

std::vector<explosion_definition> read_explosions(bytes source);
std::vector<debris_definition> read_debris(bytes source);

} // namespace earthsiege
