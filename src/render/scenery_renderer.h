#pragma once
#include <span>
#include <vector>
#include "render/distance_shading.h"
#include "render/effect_lighting.h"
#include "render/model_renderer.h"
#include "simulation/scenery_world.h"

namespace earthsiege {

/// A visible moving model; ownership remains with its actor and resource catalogue
struct scene_model {
  std::span<model_part const> roots;
  model_instance const &instance;
  model_transform placement;
  int16_t native_class{0};
  std::span<bitmap const> textures;
  std::span<model_attachment const> attachments;
  std::span<int32_t const> detail_thresholds;
  model_detail detail;

};

struct scene_ribbon {
  vec3<int32_t> position;
  std::span<vec3<int32_t> const> points;
  std::span<std::array<uint16_t, 4> const> faces;
  uint8_t colour{0};
};

using scene_object = std::variant<scene_model, scene_ribbon>;

/// The original flat-effect pass precedes terrain/actor sorting and places Z at ground on draw.
struct ground_flash {
  model_part const &root;
  model_instance const &instance;
  vec3<int32_t> &position;
};

/// Render owned scenery with the original distance ramps and whole-scene painter tree
enum class scene_shading { distance, cockpit_map };

class scenery_renderer {
  framebuffer &target;
  scenery_world const &world;
  palette colours;
  distance_shading shading;
  model_renderer models;
  scene_shading palette_mode;

public:
  scenery_renderer(framebuffer &target, scenery_world const &world, distance_shade_bindings bindings, scene_shading palette_mode = scene_shading::distance);
  std::vector<size_t> draw(vec3<int32_t> const &position, vec3<uint16_t> const &angles, std::span<light const> lights, bool split_highlights = false, std::span<scene_model const> actors = {}, effect_lighting *dynamic = nullptr, std::span<ground_flash const> ground = {}, model_view view = {});
  std::vector<size_t> draw_objects(vec3<int32_t> const &position, vec3<uint16_t> const &angles, std::span<light const> lights, bool split_highlights, std::span<scene_object const> objects, effect_lighting *dynamic = nullptr, std::span<ground_flash const> ground = {}, model_view view = {});
};

} // namespace earthsiege
