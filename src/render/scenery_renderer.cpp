#include "scenery_renderer.h"
#include <algorithm>
#include <stdexcept>
#include <utility>
#include "render/scene_order.h"
#include "render/polygon_raster.h"
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

scenery_renderer::scenery_renderer(framebuffer &buffer, scenery_world const &source, distance_shade_bindings bindings, scene_shading const mode) : target{buffer}, world{source}, colours{source.colours()}, shading{source.environment(), std::move(bindings)}, models{buffer, colours}, palette_mode{mode} {
  if(palette_mode == scene_shading::cockpit_map) apply_cockpit_map_shading(colours);
}

std::vector<size_t> scenery_renderer::draw(vec3<int32_t> const &position, vec3<uint16_t> const &angles, std::span<light const> lights, bool const split_highlights, std::span<scene_model const> const actors, effect_lighting * const dynamic, std::span<ground_flash const> const ground, model_view view) {
  std::vector<scene_object> visible;
  visible.reserve(actors.size());
  for(auto const &actor : actors) visible.emplace_back(actor);
  return draw_objects(position, angles, lights, split_highlights, visible, dynamic, ground, view);
}

std::vector<size_t> scenery_renderer::draw_objects(vec3<int32_t> const &position, vec3<uint16_t> const &angles, std::span<light const> lights, bool const split_highlights, std::span<scene_object const> const actors, effect_lighting * const dynamic, std::span<ground_flash const> const ground, model_view view) {
  auto camera{rotation_transform(angles)};
  camera.translation = position;
  view.camera = inverse_transform(camera);
  for(auto const &flash : ground) {
    vec3<int32_t> delta;
    for(unsigned int axis{0}; axis != 3; ++axis) delta[axis] = wrap_long(static_cast<int64_t>(flash.position[axis]) - position[axis]);
    if(fast_distance(delta) >= 30000) continue;
    flash.position.z = 0;
    auto placement{rotation_transform({0,0,flash.heading})};
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
    auto const *model{std::get_if<scene_model>(&actors[index])};
    if(model && model->roots.empty()) throw std::invalid_argument{"scene actor has no model"};
    auto const &origin{model ? model->placement.translation : std::get<scene_ribbon>(actors[index]).position};
    auto const radius{model ? model->roots.front().header.radius : int16_t{0}};
    auto const native_class{model ? model->native_class : int16_t{3}};
    if(origin.z < 0) continue;
    vec3<int32_t> delta;
    for(unsigned int axis{0}; axis != 3; ++axis) delta[axis] = wrap_long(static_cast<int64_t>(origin[axis]) - position[axis]);
    auto const distance{fast_distance(delta)};
    if(shading.culled(distance, native_class, radius)) continue;
    entries.push_back({.position{vec3<int32_t>{origin}}, .distance{distance}, .segment{std::nullopt}});
    indices.push_back(objects.size() + index);
  }
  auto order{scene_painter_order(entries, position)};
  for(auto &index : order) {
    auto const distance{entries[index].distance};
    index = indices[index];
    if(index >= objects.size()) {
      auto const &object{actors[index - objects.size()]};
      if(auto const *ribbon{std::get_if<scene_ribbon>(&object)}) {
        if(dynamic) dynamic->select(ribbon->position, 0);
        if(palette_mode == scene_shading::distance) shading.apply(colours, distance, 3, 0, split_highlights);
        for(auto const &face : ribbon->faces) {
          std::array<camera_vertex, 4> vertices;
          for(size_t point{0}; point != face.size(); ++point) vertices[point].position = world_point(ribbon->points[face[point]], view.camera);
          auto const projected{project_polygon(vertices, view.projection, projection_source::model)};
          std::vector<vec2<int32_t>> points;
          for(auto const &vertex : projected) points.push_back(vertex.position);
          // The beam's two-sided fill detects winding before viewport clipping,
          // reversing zero-area projected strips too (unlike model back-face tests).
          int64_t area{0};
          if(points.size() > 2) {
            auto const *previous{&points.back()};
            for(auto const &point : points) {
              area += static_cast<int64_t>(previous->x) * point.y - static_cast<int64_t>(point.x) * previous->y;
              previous = &point;
            }
            if(area < (int64_t{1} << 32) && (area < 0 || wrap_long(area) < 1)) std::ranges::reverse(points);
          }
          draw_solid_polygon(target, points, view.bounds, ribbon->colour, polygon_winding::preserve);
        }
        continue;
      }
      auto const &actor{std::get<scene_model>(object)};
      auto const radius{actor.roots.front().header.radius};
      if(palette_mode == scene_shading::distance) shading.apply(colours, distance, actor.native_class, radius, split_highlights);
      auto const object_lights{dynamic ? dynamic->select(actor.placement.translation, radius) : lights};
      auto const &placement{actor.placement};
      auto const eye{inverse_transform(compose_transforms(placement, view.camera)).translation};
      auto detail{shapes?shapes->actor:actor.detail};
      detail.projection_shift = view.projection.perspective_shift;
      auto const root{actor.roots.size() == 1 ? 0 : actor_detail_index(actor.roots.size(), actor.detail_thresholds, radius, eye, detail)};
      auto actor_view{view};
      if(shapes){
        actor_view.weapon_detail_bias=shapes->weapon;
        if(actor.native_class==7)actor_view.detail.bias=shapes->herc;
        else if(actor.native_class==6)actor_view.detail.bias=shapes->ordinary;
      }
      if(observe)observe({vec3<int32_t>{placement.translation},actor.native_class,distance,radius,root,object_lights});
      models.draw(actor.roots[root], actor.instance, placement, actor_view, actor.textures, object_lights, actor.attachments);
      continue;
    }
    auto const &object{objects[index]};
    auto const &root{world.terrain().roots.at(object.shape)};
    if(palette_mode == scene_shading::distance) shading.apply(colours, distance, 0, root.header.radius, split_highlights);
    auto const object_lights{dynamic ? dynamic->select(object.placement.translation, root.header.radius) : lights};
    models.draw(root, object.model, object.placement, view, world.textures(), object_lights);
  }
  return order;
}

} // namespace earthsiege
