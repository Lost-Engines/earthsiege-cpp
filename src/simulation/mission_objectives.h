#pragma once
#include <functional>
#include <optional>
#include <span>
#include "resources/mission_script.h"
#include "vectorstorm/vector/vector2.h"
#include "simulation/frame_time.h"

namespace earthsiege {

// These values index the original mission-status text and response tables.
enum class mission_status : int16_t {
  none = -1, dead = 2, disabled = 3, threatened = 4, objective_critical = 6,
  boundary_warning = 7, outside_area = 8, complete = 9, complete_under_threat = 10,
  route_incomplete = 11, reconnaissance_incomplete = 12, protection_incomplete = 13,
  escort_incomplete = 14, rescue_incomplete = 15, attack_incomplete = 16,
  abandoned_disabled = 18,
};

struct mission_bounds {
  vec2<int32_t> minimum{};
  vec2<int32_t> maximum{};
  static mission_bounds from_points(std::span<vec3<int32_t> const> points);
  bool outside(vec3<int32_t> const &position, int32_t margin) const;
};

struct objective_actor {
  vec3<int32_t> position{};
  size_t group{0};
  int16_t damage_fraction{0};
  bool dead{false};
  bool disabled{false};
  bool removed{false};
  bool unavailable{false};
  bool seen{false};
  bool arrived{false};
  bool rescued{false};
  bool alerted{false};
  bool route_checked{false};
  std::optional<size_t> selected{};
  std::span<uint8_t const> known{};
};
struct objective_group {
  int16_t order{0};
  uint8_t side{0};
  bool waiting{false};
  bool route_complete{true};
  std::span<size_t const> members{};
  std::optional<size_t> target_actor{};
  std::optional<size_t> target_group{};
};

/// Read-only mission facts shared by completion checks and debrief grading.
struct objective_world {
  std::span<objective_actor const> actors;
  std::span<objective_group const> groups;
  size_t player{0};
  mission_bounds bounds;
  int16_t actor_grade(size_t actor) const;
  int16_t group_grade(size_t group) const;
  int16_t target_grade(size_t group) const;
  bool safe(size_t actor) const;
  bool target_safe(size_t group) const;
  bool target_route_complete(size_t group) const;
  mission_status status(int16_t kind, bool requested = false) const;
  int16_t result_grade(size_t group, int16_t kind) const;
};

struct mission_outcome_clock {
  int16_t poll_remaining{0};
  int16_t confirm_remaining{0};
  bool pending{false};
  mission_status reported{mission_status::none};
  std::optional<mission_status> advance(frame_ratio ratio, std::function<mission_status()> const &evaluate);
};

struct player_objective_progress {
  bool arrived{false};
  bool rescued{false};
  bool target_announced{false};
  int32_t rescue_stage{0};
  int32_t rescue_remaining{0};
};
struct player_objective_input {
  bool leader{false};
  int16_t kind{0};
  vec3<int32_t> position{};
  uint16_t heading{0};
  int16_t torso{0};
  bool selected_objective{false};
  vec3<int32_t> anchor{};
  std::optional<vec3<int32_t>> rescue_position{};
  bool target_dead{false};
  bool target_waiting{false};
};
void advance_player_objective(player_objective_progress &progress, player_objective_input const &input, mission_route const *route, std::span<vec3<int32_t> const> points, int16_t &cursor, frame_ratio ratio, std::function<void(int16_t)> const &announce);
} // namespace earthsiege
