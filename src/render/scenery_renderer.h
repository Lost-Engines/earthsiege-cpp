#pragma once
#include <span>
#include <vector>
#include "render/distance_shading.h"
#include "render/model_renderer.h"
#include "simulation/scenery_world.h"

namespace earthsiege {

/// Render owned scenery with the original distance ramps and whole-scene painter tree
class scenery_renderer {
  scenery_world const &world;
  palette colours;
  distance_shading shading;
  model_renderer models;

public:
  scenery_renderer(framebuffer &target, scenery_world const &world, distance_shade_bindings bindings);
  std::vector<size_t> draw(vec3<int32_t> const &position, vec3<uint16_t> const &angles, std::span<light const> lights, bool split_highlights = false);
};

} // namespace earthsiege
