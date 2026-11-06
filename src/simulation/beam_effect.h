#pragma once
#include <array>
#include <vector>
#include "simulation/frame_time.h"
#include "simulation/projectile_instance.h"
#include "simulation/random_state.h"

namespace earthsiege {

struct beam_effect {
  vec3<int32_t> position;
  std::vector<vec3<int32_t>> points;
  std::vector<std::array<uint16_t, 4>> faces;
  uint8_t colour{0};
  int16_t remaining{0};
};

/// Straight crossed ribbons and the ELF's jittered strip share the original projectile pool.
beam_effect make_beam_effect(int16_t subtype, beam_path const &path, random_state &random, std::span<beam_style const> styles = {}, game_edition edition = game_edition::original);
bool advance_beam_effect(beam_effect &beam, frame_ratio ratio);

} // namespace earthsiege
