#pragma once
#include <optional>
#include <span>
#include "resources/effects.h"
#include "simulation/frame_time.h"
#include "simulation/random_state.h"
#include "vectorstorm/vector/vector3.h"

namespace earthsiege {

struct explosion_state {
  vec3<int32_t> position;
  vec3<uint16_t> angles;
  int16_t remaining{0};
  std::vector<uint16_t> cells;
  std::optional<uint16_t> secondary_cell;
  std::optional<int16_t> light_intensity;
  int16_t sound{0};
};

struct debris_particle {
  int16_t model{0};
  vec3<int32_t> position;
  vec3<uint16_t> angles;
  vec3<int16_t> velocity;
};

/// Caller reserves pool capacity before creation; a failed original allocation consumes no random draw
explosion_state create_explosion(explosion_definition const &definition, size_t cells, vec3<int32_t> const &position, random_state &random);
/// Return true at expiry, retaining original overshoot discard and light/cell update order
bool advance_explosion(explosion_state &state, explosion_definition const &definition, std::span<int16_t const> frame_counts, uint16_t secondary_count, frame_ratio ratio);
std::vector<debris_particle> create_debris_burst(debris_definition const &definition, vec3<int32_t> const &position, random_state &random);

} // namespace earthsiege
