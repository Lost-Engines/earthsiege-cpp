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
  std::optional<unsigned> weapon_detail_bias;
};

/// Replace a body placeholder using the body skeleton and independent weapon cells/textures
struct model_attachment {
  int16_t body_part{0};
  model_part const *geometry{nullptr};
  std::span<uint16_t const> cells;
  std::span<bitmap const> textures;
};

class model_renderer {
  framebuffer &target;
  palette const &colours;
  texture_shade_maps texture_shades;

public:
  model_renderer(framebuffer &target, palette const &colours);
  void draw(model_part const &source, model_instance const &instance, model_transform const &object, model_view const &view, std::span<bitmap const> textures, std::span<light const> lights, std::span<model_attachment const> attachments = {});
};

} // namespace earthsiege
