#pragma once
#include <optional>
#include "render/framebuffer.h"
#include "render/projection.h"
#include "resources/model.h"

namespace earthsiege {
struct hud_target_state {
  vec3<int32_t> position;
  int32_t model_radius;
  uint8_t side;
  bool visible{true};
  bool locked{false};
};
/// Project the original two vertical world points, then limit bracket size.
std::optional<rectangle> target_brackets(hud_target_state const &state, model_transform const &world_to_camera, camera_projection const &projection);
void draw_target_brackets(framebuffer &target, rectangle const &bounds, bool locked, uint8_t side);
} // namespace earthsiege
