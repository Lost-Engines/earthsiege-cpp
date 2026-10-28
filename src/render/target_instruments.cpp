#include "target_instruments.h"
#include <algorithm>
#include <array>
#include "render/polygon_raster.h"
#include "simulation/fixed_math.h"
#include "simulation/model_math.h"

namespace earthsiege {
std::optional<rectangle> target_brackets(hud_target_state const &state, model_transform const &world_to_camera, camera_projection const &projection) {
  if(!state.visible) return {};
  auto lower{state.position}, upper{state.position};
  lower.z = wrap_long(static_cast<int64_t>(lower.z) - (state.model_radius >> 1));
  upper.z = wrap_long(static_cast<int64_t>(lower.z) + static_cast<int64_t>(state.model_radius) * 2);
  auto const transform{[&](vec3<int32_t> const &point){
    auto value{rotate_vector(point, world_to_camera)};
    for(unsigned int i{}; i < 3; ++i) value[i] = wrap_long(static_cast<int64_t>(value[i]) + world_to_camera.translation[i]);
    return value;
  }};
  lower = transform(lower); upper = transform(upper);
  // 0x340c0 accepts the near plane itself, independently of model clipping.
  if(lower.y < 256 || upper.y < 256) return {};
  auto bottom{project_point(lower, projection, projection_source::model)};
  auto top{project_point(upper, projection, projection_source::model)};
  int size{std::abs(bottom.y - top.y)};
  if(size < 25) { int const pad{(25 - size) >> 1}; top.y -= pad; bottom.y += pad; size = 25; }
  else if(size > 75) { int const trim{(size - 75) >> 1}; top.y += trim; bottom.y -= trim; size = 75; }
  return rectangle{top.x - (size >> 1), top.y, top.x + (size >> 1), bottom.y};
}
void draw_target_brackets(framebuffer &target, rectangle const &bounds, bool const locked, uint8_t const side) {
  int const l{bounds.left}, t{bounds.top}, r{bounds.right}, b{bounds.bottom};
  int const x{r - l <= 6 ? (r - l) >> 1 : 6};
  int const y{b - t <= 6 ? (b - t) >> 1 : 6};
  uint8_t const colour{static_cast<uint8_t>(side ? 53 : 1)};
  if(!locked) {
    target.line(l,t,l,t+y,colour); target.line(l,t,l+x,t,colour);
    target.line(r-x,t,r,t,colour); target.line(r,t,r,t+y,colour);
    target.line(r,b-y,r,b,colour); target.line(r-x,b,r,b,colour);
    target.line(l,b-y,l,b,colour); target.line(l,b,l+x,b,colour);
  } else {
    std::array<std::array<vec2<int32_t>,3>,4> const triangles{{
      {{{l,t+y},{l+x,t},{l+x,t+y}}},
      {{{r-x,t+y},{r-x,t},{r,t+y}}},
      {{{r-x,b},{r-x,b-y},{r,b-y}}},
      {{{l,b-y},{l+x,b-y},{l+x,b}}},
    }};
    for(auto const &triangle : triangles) draw_solid_polygon(target,triangle,{0,0,319,199},colour);
  }
}
} // namespace earthsiege
