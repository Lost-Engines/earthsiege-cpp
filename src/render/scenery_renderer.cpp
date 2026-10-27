#include "scenery_renderer.h"
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

std::vector<size_t> scenery_renderer::draw(vec3<int32_t> const &position, vec3<uint16_t> const &angles, std::span<light const> lights, bool const split_highlights) {
  auto camera{rotation_transform(angles)};
  camera.translation = position;
  model_view view;
  view.camera = inverse_transform(camera);
  std::vector<scene_order_entry> entries;
  std::vector<size_t> indices;
  auto const objects{world.objects()};
  entries.reserve(objects.size());
  indices.reserve(objects.size());
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
  auto order{scene_painter_order(entries, position)};
  for(auto &index : order) {
    auto const distance{entries[index].distance};
    index = indices[index];
    auto const &object{objects[index]};
    auto const &root{world.terrain().roots.at(object.shape)};
    shading.apply(colours, distance, 0, root.header.radius, split_highlights);
    models.draw(root, object.model, object.placement, view, world.textures(), lights);
  }
  return order;
}

} // namespace earthsiege
