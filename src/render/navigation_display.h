#pragma once
#include "render/model_renderer.h"
#include "simulation/scenery_world.h"

namespace earthsiege {

/// The NAV MAP has its own overhead camera and ramps, and draws the static zone in resource order.
class navigation_display {
  framebuffer &target;
  scenery_world const &world;
  palette colours;
  model_renderer models;
public:
  navigation_display(framebuffer &target, scenery_world const &world);
  void draw(rectangle const &bounds, vec3<int32_t> const &position, vec3<uint16_t> const &angles, std::span<light const> lights);
};

} // namespace earthsiege
