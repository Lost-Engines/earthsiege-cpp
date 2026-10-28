#pragma once
#include <optional>
#include "simulation/fixed_math.h"

namespace earthsiege {

enum class control_release_action : uint8_t { none, stop, centre };
struct player_input {
  vec2<int16_t> movement{};
  vec2<int16_t> aiming{};
  vec2<int16_t> camera{};
  vec2<int32_t> mouse{};                                                    // original 640×480 cockpit coordinates
  bool left{false};
  bool right{false};
  bool fire{false};
  bool enter{false};
  bool released{false};
};
struct player_input_target {
  vec3<int32_t> position{};
  vec3<int16_t> offset{};                                                   // original virtual target offset, added in world axes
  bool dead{false};
};
struct player_input_context {
  bool external_control{false};
  bool controlled_flyer{false};
  bool active{true};
  bool disabled{false};
  bool auto_tracking{false};
  vec3<int32_t> position{};
  std::optional<player_input_target> target;
  vec2<int32_t> viewport_min{};
  vec2<int32_t> viewport_max{639, 479};
  control_release_action release_action{control_release_action::none};
};
enum class player_aim_mode : uint8_t { none, manual, centre, target };
struct player_input_actions {
  vec2<int16_t> camera{};
  bool zoom{false};
  std::optional<vec2<int16_t>> movement;
  bool move_flyer{false};
  player_aim_mode aim{player_aim_mode::none};
  vec2<int16_t> aiming{};
  int32_t distance{0};
  vec3<int32_t> target_point{};
  bool reset_aim{false};
  bool fire_allowed{false};
  bool fire{false};
  bool cycle_target{false};
  bool stop{false};
};

/// Device-independent original player routing and persistent mouse/centering state.
class player_input_router {
  bool mouse_latched{false};
  vec2<int32_t> mouse_origin{};
  vec2<int32_t> mouse_response{};
  bool centering{false};
  bool enter_held{false};
public:
  void request_centre();
  void clear_device_state();
  bool centred() const;
  bool enter_pressed() const;
  bool aiming_with_mouse() const;
  vec2<int32_t> origin() const;
  vec2<int32_t> response() const;
  player_input_actions route(player_input const &input, player_input_context const &context);
};

} // namespace earthsiege
