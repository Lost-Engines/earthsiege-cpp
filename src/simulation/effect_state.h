#pragma once
#include <optional>
#include <span>
#include "resources/effects.h"
#include "resources/weapon_mounts.h"
#include "resources/model.h"
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

struct smoke_emitter_state {
  int16_t family{}, remaining{}, timer{}, root{};
};
struct smoke_puff_state {
  int16_t root{};
  vec3<int32_t> position;
  vec3<int16_t> velocity;                                                 // original constructor retains the pool slot's words
  int32_t timer{200};
  uint16_t cell{};
};
smoke_emitter_state create_smoke_emitter(int16_t family, random_state &visual_random);
std::optional<int16_t> advance_smoke_emitter(smoke_emitter_state &state, frame_ratio ratio);
bool advance_smoke_puff(smoke_puff_state &state, uint16_t frames, frame_ratio ratio);
vec3<int32_t> smoke_attachment_position(model_transform const &owner, std::optional<vec3<int16_t>> const &offset = {}, model_transform const *part = nullptr);
debris_particle create_detached_weapon(weapon_definition const &definition, weapon_mount const &mount, model_transform const &hardpoint, vec3<int32_t> const &owner_centre);

/// Caller reserves pool capacity before creation; a failed original allocation consumes no random draw
explosion_state create_explosion(explosion_definition const &definition, size_t cells, vec3<int32_t> const &position, random_state &random);
/// Return true at expiry, retaining original overshoot discard and light/cell update order
bool advance_explosion(explosion_state &state, explosion_definition const &definition, std::span<int16_t const> frame_counts, uint16_t secondary_count, frame_ratio ratio);
/// Original debris advances once per update, without the Q8 frame ratio; true means retire
bool advance_debris(debris_particle &state);
std::vector<debris_particle> create_debris_burst(debris_definition const &definition, vec3<int32_t> const &position, random_state &random);

} // namespace earthsiege
