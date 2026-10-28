#include "scenery_renderer.h"
#include <stdexcept>
#include <utility>
#include "render/scene_order.h"
#include "simulation/fixed_math.h"
#include "simulation/model_math.h"

namespace earthsiege {
namespace {

vec3<int32_t> world_point(vec3<int32_t> const &point, model_transform const &placement) {
  auto result{rotate_vector(point, placement)};
  for(unsigned int axis{0}; axis != 3; ++axis) result[axis] = wrap_long(static_cast<int64_t>(result[axis]) + placement.translation[axis]);
  return result;
}

} // anonymous namespace

scenery_renderer::scenery_renderer(framebuffer &target, scenery_world const &source, distance_shade_bindings bindings) : world{source}, colours{source.colours()}, shading{source.environment(), std::move(bindings)}, models{target, colours} {
}

std::vector<size_t> scenery_renderer::draw(vec3<int32_t> const &position, vec3<uint16_t> const &angles, std::span<light const> lights, bool const split_highlights, std::span<scene_model const> const actors, effect_lighting * const dynamic, std::span<ground_flash const> const ground) {
  auto camera{rotation_transform(angles)};
  camera.translation = position;
  model_view view;
  view.camera = inverse_transform(camera);
  for(auto const &flash : ground) {
    vec3<int32_t> delta;
    for(unsigned int axis{0}; axis != 3; ++axis) delta[axis] = wrap_long(static_cast<int64_t>(flash.position[axis]) - position[axis]);
    if(fast_distance(delta) >= 30000) continue;
    flash.position.z = 0;
    model_transform placement;
    placement.translation = flash.position;
    models.draw(flash.root, flash.instance, placement, view, {}, dynamic ? dynamic->current() : lights);
  }
  std::vector<scene_order_entry> entries;
  std::vector<size_t> indices;
  auto const objects{world.objects()};
  entries.reserve(objects.size() + actors.size());
  indices.reserve(objects.size() + actors.size());
  for(size_t index{0}; index != objects.size(); ++index) {
    auto const &object{objects[index]};
    auto const &origin{object.placement.translation};
    if(origin.z < 0) continue;
    vec3<int32_t> delta;
    for(unsigned int axis{0}; axis != 3; ++axis) delta[axis] = wrap_long(static_cast<int64_t>(origin[axis]) - position[axis]);
    auto const distance{fast_distance(delta)};
    auto const radius{world.terrain().roots.at(object.shape).header.radius};
    if(shading.culled(distance, 0, radius)) continue;
    scene_order_entry entry{.position{vec3<int32_t>{origin}}, .distance{distance}, .segment{std::nullopt}};
    if(object.metadata) entry.segment = scene_sort_segment{.first{world_point(object.metadata->sort_endpoints[0], object.placement)}, .second{world_point(object.metadata->sort_endpoints[1], object.placement)}};
    entries.push_back(entry);
    indices.push_back(index);
  }
  for(size_t index{0}; index != actors.size(); ++index) {
    auto const &actor{actors[index]};
    if(actor.roots.empty()) throw std::invalid_argument{"scene actor has no model"};
    auto const &origin{actor.placement.translation};
    if(origin.z < 0) continue;
    vec3<int32_t> delta;
    for(unsigned int axis{0}; axis != 3; ++axis) delta[axis] = wrap_long(static_cast<int64_t>(origin[axis]) - position[axis]);
    auto const distance{fast_distance(delta)};
    if(shading.culled(distance, actor.native_class, actor.roots.front().header.radius)) continue;
    entries.push_back({.position{vec3<int32_t>{origin}}, .distance{distance}, .segment{std::nullopt}});
    indices.push_back(objects.size() + index);
  }
  auto order{scene_painter_order(entries, position)};
  for(auto &index : order) {
    auto const distance{entries[index].distance};
    index = indices[index];
    if(index >= objects.size()) {
      auto const &actor{actors[index - objects.size()]};
      auto const radius{actor.roots.front().header.radius};
      shading.apply(colours, distance, actor.native_class, radius, split_highlights);
      auto const object_lights{dynamic ? dynamic->select(actor.placement.translation, radius) : lights};
      auto const &placement{actor.placement};
      auto const eye{inverse_transform(compose_transforms(placement, view.camera)).translation};
      auto detail{actor.detail};
      detail.projection_shift = view.projection.perspective_shift;
      auto const root{actor.roots.size() == 1 ? 0 : actor_detail_index(actor.roots.size(), actor.detail_thresholds, radius, eye, detail)};
      models.draw(actor.roots[root], actor.instance, placement, view, actor.textures, object_lights, actor.attachments);
      continue;
    }
    auto const &object{objects[index]};
    auto const &root{world.terrain().roots.at(object.shape)};
    shading.apply(colours, distance, 0, root.header.radius, split_highlights);
    auto const object_lights{dynamic ? dynamic->select(object.placement.translation, root.header.radius) : lights};
    models.draw(root, object.model, object.placement, view, world.textures(), object_lights);
  }
  return order;
}

} // namespace earthsiege
