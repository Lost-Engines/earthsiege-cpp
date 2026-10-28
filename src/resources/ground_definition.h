#pragma once
#include <array>
#include <span>
#include <vector>
#include "resources/artwork.h"
#include "resources/combat.h"
#include "resources/model.h"

namespace earthsiege {

class resource_store;

struct ground_profile {
  int16_t type{0};
  int16_t model{0};
  int16_t wreck_model{0};
  int16_t channels{0};
  int16_t damage_capacity{0};
  vec3<int16_t> destruction_centre;
  vec3<int16_t> burst_centre;
  vec3<int16_t> burst_half_extent;
  vec3<int16_t> weak_point;
  int16_t weak_point_radius{0};
  std::array<int16_t, 2> animation_rates{};
  int16_t cell_sequence{-1};
  int16_t cell_period{0};
  int16_t contact_radius{0};
  int16_t target_height{0};
  std::array<int16_t, 27> source{};                                          // remaining fields retained until their consumers are ported
  std::vector<collision_part> collision;
};

/// Side-specific static/animated bases share a texture bank and destroyed models.
class ground_catalogue {
  std::vector<ground_profile> definitions;
  std::array<model_resource, 2> static_models;
  std::array<model_resource, 2> animated_models;
  model_resource wrecks;
  std::vector<bitmap> texture_bank;

public:
  explicit ground_catalogue(resource_store const &resources);
  std::span<ground_profile const> profiles() const;
  ground_profile const &profile(size_t type) const;
  model_part const &model(size_t type, uint8_t side, bool destroyed = false) const;
  std::span<bitmap const> textures() const;
};

} // namespace earthsiege
