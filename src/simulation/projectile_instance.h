#pragma once
#include <functional>
#include <optional>
#include "simulation/model_instance.h"
#include "simulation/projectile_flight.h"
#include "simulation/weapon_firing.h"

namespace earthsiege {

struct impact_payload {
  int16_t armour{0};
  int16_t shield{0};
  int16_t splash_q10{0};
  std::array<int16_t, 4> shield_effects{};
  std::array<int16_t, 4> ground_effects{};
  std::array<int16_t, 4> armour_effects{};
  std::optional<uint8_t> missile_subtype;
};

struct impact_request {
  model_transform transform;
  int32_t reach{0};
  int16_t radius{0};
  impact_payload damage;
};

/// Returns the retained world-hit distance; an empty result means no impact
using impact_query = std::function<std::optional<int32_t>(impact_request const &)>;
impact_payload projectile_payload(projectile_definition const &definition, int16_t charge);

struct beam_path {
  vec3<int32_t> start;
  vec3<int32_t> end;
};

beam_path dispatch_beam(weapon_shot const &shot, weapon_catalogue const &definitions, impact_query const &hit);

/// Own one spawned bullet/rocket and its visible cells; owner/target identity belongs to the world
class projectile_instance {
  projectile_definition const &source;
  projectile_motions const &motions;
  model_part const &geometry_source;
  shape_part const &shape;
  projectile_flight flight_state;
  std::vector<uint16_t> cell_counts;
  model_instance visual;
  int16_t damage_scale{0};

public:
  projectile_instance(weapon_shot const &shot, weapon_catalogue const &definitions, projectile_motions const &motions, model_resource const &bullet_models, model_resource const &rocket_models, bool target_present = false, int16_t initial_rocket_clock = 0);
  projectile_flight const &flight() const;
  model_instance const &model() const;
  model_part const &geometry() const;
  void clear_target();
  model_transform placement() const;
  projectile_result advance(frame_ratio ratio, projectile_guidance &guidance, impact_query const &hit);
};

} // namespace earthsiege
