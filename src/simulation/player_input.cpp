#include "player_input.h"

namespace earthsiege {
void player_input_router::request_centre() { centering = true; }
void player_input_router::clear_device_state() { mouse_latched = false; mouse_response = {}; enter_held = false; }
bool player_input_router::centred() const { return centering; }
bool player_input_router::enter_pressed() const { return enter_held; }
bool player_input_router::aiming_with_mouse() const { return mouse_latched; }
vec2<int32_t> player_input_router::origin() const { return vec2<int32_t>{mouse_origin}; }
vec2<int32_t> player_input_router::response() const { return vec2<int32_t>{mouse_response}; }

player_input_actions player_input_router::route(player_input const &input, player_input_context const &context) {
  player_input_actions result;
  auto movement{context.external_control ? vec2<int16_t>{} : input.movement};
  result.aiming = context.external_control ? vec2<int16_t>{} : input.aiming;
  if(context.external_control) { result.camera = input.camera; result.zoom = input.fire; }
  if(!input.right) {
    mouse_latched = false;
    mouse_response = {};
  } else if(!input.left) {
    if(!mouse_latched) {
      if(input.mouse.x >= context.viewport_min.x && input.mouse.x <= context.viewport_max.x && input.mouse.y >= context.viewport_min.y && input.mouse.y <= context.viewport_max.y) {
        mouse_latched = true;
        mouse_origin = input.mouse;
      }
    } else {
      auto const x{wrap_long(static_cast<int64_t>(mouse_origin.x) - input.mouse.x)};
      auto const y{wrap_long(static_cast<int64_t>(mouse_origin.y) - input.mouse.y)};
      mouse_response.x = wrap_long(static_cast<int64_t>(x) * x) >> 6;
      mouse_response.y = wrap_long(static_cast<int64_t>(y) * y) >> 5;
      if(x > 0) mouse_response.x = wrap_long(-static_cast<int64_t>(mouse_response.x));
      if(y < 0) mouse_response.y = wrap_long(-static_cast<int64_t>(mouse_response.y));
    }
  } else {
    result.reset_aim = true;
    mouse_latched = false;
    // Both buttons reset the angles but retain this update's preceding response.
  }
  result.aiming.x = wrap_word(static_cast<int32_t>(result.aiming.x) + wrap_word(mouse_response.x));
  result.aiming.y = wrap_word(static_cast<int32_t>(result.aiming.y) + wrap_word(mouse_response.y));
  if(context.controlled_flyer && !context.external_control) {
    result.movement = movement;
    result.move_flyer = true;
  } else if(context.active) {
    result.movement = movement;
    if(context.target) {
      vec3<int32_t> delta;
      for(unsigned int axis{0}; axis != 3; ++axis) delta[axis] = wrap_long(static_cast<int64_t>(context.target->position[axis]) - context.position[axis]);
      result.distance = fast_distance(delta);
    }
    if(context.disabled) centering = true;
    else if(result.aiming.x == 0 && result.aiming.y == 0) {
      if(context.auto_tracking && context.target && !context.target->dead) {
        result.aim = player_aim_mode::target;
        for(unsigned int axis{0}; axis != 3; ++axis) result.target_point[axis] = wrap_long(static_cast<int64_t>(context.target->position[axis]) + context.target->offset[axis]);
        centering = false;
      }
    } else centering = false;
    if(centering) result.aim = player_aim_mode::centre;
    else if(result.aim != player_aim_mode::target) result.aim = player_aim_mode::manual;
  }
  result.fire_allowed = !context.external_control;
  result.fire = input.fire;
  result.cycle_target = input.enter && !enter_held;
  enter_held = input.enter;
  if(input.released) {
    if(context.release_action == control_release_action::stop) result.stop = true;
    else if(context.release_action == control_release_action::centre) centering = true;
  }
  return result;
}
} // namespace earthsiege
