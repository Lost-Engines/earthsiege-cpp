#pragma once
#include <span>
#include "render/framebuffer.h"
#include "render/model_lighting.h"
#include "render/model_traversal.h"
#include "render/palette_shading.h"
#include "render/projection.h"
#include "simulation/model_instance.h"

namespace earthsiege {

struct model_view {
  model_transform camera;
  camera_projection projection;
  rectangle bounds{0, 0, 319, 199};
  model_detail detail;
};

class model_renderer {
  framebuffer &target;
  palette const &colours;
  texture_shade_maps texture_shades;

public:
  model_renderer(framebuffer &target, palette const &colours);
  void draw(model_part const &source, model_instance const &instance, model_transform const &object, model_view const &view, std::span<bitmap const> textures, std::span<light const> lights);
};

} // namespace earthsiege
