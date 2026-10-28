#include "waypoint_instruments.h"
#include <array>
#include <string>
#include "render/polygon_raster.h"
#include "simulation/fixed_math.h"
#include "simulation/model_math.h"

namespace earthsiege {
waypoint_instruments::waypoint_instruments(resource_store const &resources, cockpit_layout const &layout)
  : bounds{layout.bounds(0x460)}, face{read_font(resources.read("simvol0", "dfn/CPGREEN.DFN"))} {
  auto const parent{layout.bounds(0x450)};
  bounds.left += parent.left; bounds.right += parent.left;
  bounds.top += parent.top; bounds.bottom += parent.top;
}
void waypoint_instruments::draw(framebuffer &target, vec3<int32_t> const &position, uint16_t const heading, uint16_t const tape_heading,
                               std::optional<hud_waypoint_state> const &waypoint, int const offset_x, int const offset_y) const {
  if(!waypoint) return;
  scoped_framebuffer_clip clip{target,nullptr};
  auto const bearing{bearing_angle(position,waypoint->position)};
  auto const difference{static_cast<uint16_t>(heading-bearing)};
  bool const right{difference >= 0xe39 && difference <= 0x7fff};
  bool const left{difference > 0x7fff && difference < 0xf1c8};
  int const l{bounds.left + offset_x}, r{bounds.right + offset_x}, t{bounds.top + offset_y};
  std::array<vec2<int32_t>,3> arrow;
  if(left) arrow={vec2<int32_t>{l,t+6},{l,t+10},{l-2,t+8}};
  else if(right) arrow={vec2<int32_t>{r,t+6},{r+2,t+8},{r,t+10}};
  else {
    int const fraction{static_cast<uint16_t>(bearing - static_cast<uint16_t>(tape_heading-3640))};
    int const x{r - fraction * (bounds.right - bounds.left) / 7280};
    arrow={vec2<int32_t>{x,t+8},{x-2,t+5},{x+2,t+5}};
  }
  draw_solid_polygon(target,arrow,{0,0,319,199},1);
  vec2<int32_t> const delta{wrap_long(static_cast<int64_t>(waypoint->position.x)-position.x),wrap_long(static_cast<int64_t>(waypoint->position.y)-position.y)};
  int const metres{fast_distance_2d(delta)/1000*6};
  auto const label{"WP " + std::to_string(waypoint->number) + ": " + std::to_string(metres) + " M."};
  target.text(face,label,l+((bounds.right-bounds.left)>>1)-((face.text_width(label)-1)>>1),
              t+((bounds.bottom-bounds.top+static_cast<int>(face.height))>>1)+6);
}
} // namespace earthsiege
