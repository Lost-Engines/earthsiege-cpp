#pragma once
#include <functional>
#include <span>
#include <vector>
#include "resources/combat.h"
#include "resources/model.h"
#include "resources/projectile_motion.h"
#include "simulation/frame_time.h"

namespace earthsiege {

inline int16_t constexpr rocket_start_speed{500};
inline int16_t constexpr rocket_turn_limit{1280};
inline int16_t constexpr rocket_interference_angle{3072};
inline uint8_t constexpr rocket_target_sensor_subtype{2};
inline uint8_t constexpr rocket_manual_subtype{3};

struct projectile_flight {
  projectile_kind kind{projectile_kind::bullet};
  uint8_t subtype{0};
  vec3<int32_t> position;
  vec3<uint16_t> angles;
  int16_t speed{0};
  int16_t age{0};
  int16_t animation_remaining{0};
  std::vector<uint16_t> cells;
  bool target_present{false};
};

/// Target location is resolved from the selected model node or actor origin by the owner
struct projectile_guidance {
  vec3<int32_t> target_position;
  bool player_owner{false};
  bool owner_tracks_target{false};
  bool target_detectable{false};
  bool interference{false};
  bool manual_held{false};
  vec2<int16_t> manual_axes;
  bool owner_guiding{false};
  bool manual_input_used{false};                                          // player steering consumer ran, even when releasing guidance
};

enum class projectile_result { flying, expired, impact };
using projectile_contact = std::function<bool(model_transform const &, int16_t, int16_t)>;

/// Advance original cells, lifetime and guidance; dispatch the ray before committing movement
projectile_result advance_projectile(projectile_flight &state, projectile_motions const &motions, int16_t speed_limit, std::span<uint16_t const> cell_counts, frame_ratio ratio, projectile_guidance &guidance, projectile_contact const &contact = {});

} // namespace earthsiege
