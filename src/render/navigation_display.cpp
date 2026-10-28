#include "navigation_display.h"
#include <numeric>
#include "simulation/model_math.h"

namespace earthsiege {
namespace {
palette map_palette(palette result) {
  for(size_t ramp{}; ramp < 16; ramp += 2) {
    auto &values{result.shade_ramps.at(ramp)};
    values.resize(16);
    std::iota(values.begin(), values.end(), uint16_t{16});
  }
  return result;
}
} // namespace

navigation_display::navigation_display(framebuffer &buffer, scenery_world const &source)
  : target{buffer}, world{source}, colours{map_palette(source.colours())}, models{buffer, colours} {}

void navigation_display::draw(rectangle const &bounds, vec3<int32_t> const &position, vec3<uint16_t> const &angles, std::span<light const> lights) {
  scoped_framebuffer_clip clip{target, nullptr};
  target.fill(bounds, 23);
  auto map_position{position};
  auto map_angles{angles};
  map_position.z = 200000;
  map_angles.x = 49152;                                                         // -quarter turn: look down; retain roll and heading
  auto camera{rotation_transform(map_angles)};
  camera.translation = map_position;
  model_view view;
  view.camera = inverse_transform(camera);
  view.bounds = bounds;
  view.projection.perspective_shift = 7;
  view.projection.near_distance = 128;
  view.projection.centre = {bounds.left + ((bounds.right - bounds.left) >> 1), bounds.top + ((bounds.bottom - bounds.top) >> 1)};
  for(auto const &object : world.objects())
    models.draw(world.terrain().roots.at(object.shape), object.model, object.placement, view, world.textures(), lights);
  auto const centre{view.projection.centre};
  for(auto const &offset : std::array<vec2<int32_t>, 5>{{{0, -1}, {0, 0}, {0, 1}, {-1, 0}, {1, 0}}})
    if(bounds.contains(centre.x + offset.x, centre.y + offset.y)) target.pixel(centre.x + offset.x, centre.y + offset.y, 60);
}
} // namespace earthsiege
