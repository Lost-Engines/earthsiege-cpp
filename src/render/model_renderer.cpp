#include "model_renderer.h"
#include <algorithm>
#include <numeric>
#include <stdexcept>
#include <type_traits>
#include <vector>
#include "render/bitmap_raster.h"
#include "render/gouraud_raster.h"
#include "render/line_raster.h"
#include "render/polygon_raster.h"
#include "render/texture_raster.h"
#include "simulation/fixed_math.h"
#include "simulation/model_math.h"

namespace earthsiege {
namespace {

uint32_t packed_material(material_slot const slot) {
  return static_cast<uint16_t>(slot.value) | static_cast<uint32_t>(slot.flags) << 16;
}

class model_draw {
  framebuffer &target;
  palette const &colours;
  texture_shade_maps const &texture_shades;
  model_instance const &instance;
  model_transform const &object;
  model_view const &view;
  model_detail active_detail;
  std::span<bitmap const> textures;
  std::span<light const> lights;
  std::span<uint16_t const> cells;
  std::span<model_attachment const> attachments;

  struct part_view {
    model_transform transform;
    vec3<int32_t> eye;
    std::vector<light> lights;
  };

  part_view context(int16_t const transform_index) const {
    auto world{object};
    if(transform_index >= 0) {
      if(static_cast<size_t>(transform_index) >= instance.transforms().size()) throw std::out_of_range{"model part transform exceeds instance"};
      world = compose_transforms(instance.transforms()[static_cast<size_t>(transform_index)], object);
    }
    auto const camera{compose_transforms(world, view.camera)};
    part_view result{.transform{camera}, .eye{inverse_transform(camera).translation}, .lights{}};
    auto const inverse{inverse_transform(world)};
    result.lights.reserve(lights.size());
    for(auto const &source : lights) result.lights.push_back(transform_light(source, inverse));
    return result;
  }

  void solid(std::span<projected_vertex const> const vertices, uint32_t const material, bool const outline = false) {
    auto const mode{material >> 24};
    if(mode != 0 && mode != 4) return;
    auto const colour{mode == 4 ? shade_colour(colours.shade_ramps.at(material & 65535), static_cast<uint8_t>(material >> 16)) : static_cast<uint8_t>(material)};
    std::vector<vec2<int32_t>> points;
    points.reserve(vertices.size());
    for(auto const &vertex : vertices) points.push_back(vertex.position);
    if(points.size() == 1) {
      if(view.bounds.contains(points[0].x, points[0].y)) target.pixel(points[0].x, points[0].y, colour);
    } else if(points.size() == 2 || outline) {
      for(size_t index{1}; index < points.size(); ++index) draw_model_line(target, vec2<int32_t>{points[index]}, vec2<int32_t>{points[index - 1]}, view.bounds, colour);
      if(points.size() > 2) draw_model_line(target, vec2<int32_t>{points[0]}, vec2<int32_t>{points.back()}, view.bounds, colour);
    } else draw_solid_polygon(target, points, view.bounds, colour, polygon_winding::preserve);
  }

  void polygon(model_mesh const &mesh, model_polygon const &surface, part_view const &current) {
    if(!surface.material) return;
    auto const &material{mesh.materials.at(*surface.material)};
    auto normal{mesh.points.at(surface.normal)};
    auto const &centre{mesh.points.at(surface.centre)};
    bool back{false};
    if(view.projection.perspective_shift == 0) back = rotate_vector(vec3<int32_t>{normal}, current.transform).y >= 0;
    else {
      int64_t dot{0};
      for(unsigned int axis{0}; axis != 3; ++axis) dot += static_cast<int64_t>(normal[axis]) * wrap_long(static_cast<int64_t>(current.eye[axis]) - centre[axis]);
      back = wrap_long(dot) <= 0;
    }
    bool const textured{surface.kind == model_kind::textured_polygon};
    if(textured && packed_material(material.front_fill) == 0xffffffffU) back = true;
    auto fill{packed_material(back ? material.back_fill : material.front_fill)};
    auto line{packed_material(back ? material.back_line : material.front_line)};
    if(textured ? fill == 0xffffffffU : fill >> 24 == 20 && line >> 24 == 20) return;
    if(back) for(unsigned int axis{0}; axis != 3; ++axis) normal[axis] = wrap_word(-static_cast<int32_t>(normal[axis]));
    auto const intensity{light_intensity(current.lights, vec3<int32_t>{centre}, normal)};
    bool const shaded{surface.kind == model_kind::shaded_polygon || surface.kind == model_kind::aliased_shaded_polygon};
    bool const gouraud{surface.normal_list.has_value()};
    if(shaded) {
      fill |= static_cast<uint32_t>(intensity) << 16;
      line |= static_cast<uint32_t>(intensity) << 16;
    }
    bitmap const *texture{nullptr};
    if(textured) {
      if(fill >= textures.size()) throw std::out_of_range{"model texture index exceeds bitmap bank"};
      texture = &textures[fill];
    }
    auto const point_index{[&mesh](unsigned int const index){ return mesh.indices.empty() ? index : mesh.indices.at(index); }};
    std::vector<camera_vertex> vertices;
    vertices.reserve(surface.vertex_count);
    for(unsigned int index{0}; index != surface.vertex_count; ++index) {
      auto const &point{mesh.points.at(point_index(surface.vertex_list + index))};
      camera_vertex vertex{.position{transform_point(point, current.transform)}, .texture{0, 0}, .shade{0}};
      if(texture) {
        if(index > 3) throw std::runtime_error{"native texture surface is not a quad"};
        vertex.texture = vec2<int32_t>{index == 1 || index == 2 ? static_cast<int32_t>(texture->width) - 1 : 0, index >= 2 ? static_cast<int32_t>(texture->height) - 1 : 0};
      }
      if(gouraud) {
        auto vertex_normal{mesh.points.at(point_index(*surface.normal_list + index))};
        if(back) for(unsigned int axis{0}; axis != 3; ++axis) vertex_normal[axis] = wrap_word(-static_cast<int32_t>(vertex_normal[axis]));
        vertex.shade = light_intensity(current.lights, vec3<int32_t>{point}, vertex_normal);
      }
      vertices.push_back(vertex);
    }
    auto projected{project_polygon(vertices, view.projection, projection_source::model)};
    if(projected.empty()) return;
    if(back && projected.size() > 2) {
      /// The original reverses geometry but leaves Gouraud/translated-texture attributes in place
      for(size_t index{0}; index < projected.size() / 2; ++index) std::swap(projected[index].position, projected[projected.size() - 1 - index].position);
    }
    if(texture) draw_textured_polygon(target, projected, *texture, view.bounds, texture_shades[texture_shade_index(intensity)], target.clip() ? texture_scan_direction::rows : texture_scan_direction::columns);
    else if(gouraud && projected.size() >= 3) draw_gouraud_polygon(target, projected, colours.shade_ramps.at(fill & 65535), view.bounds);
    else solid(projected, fill);
    if(!textured && line != fill) solid(projected, line, true);
  }

  void mesh(model_mesh const &source, part_view const &current) {
    if(source.bsp.empty()) {
      for(auto const &surface : source.polygons) polygon(source, surface, current);
      return;
    }
    auto const walk{[&](this auto const &self, int16_t const reference, size_t const depth)->void {
      if(reference < 0) return;
      if(reference & 0x4000) {
        polygon(source, source.polygons.at(static_cast<unsigned int>(reference) & 0x3fff), current);
        return;
      }
      if(depth > source.bsp.size()) throw std::runtime_error{"cyclic mesh BSP"};
      auto const &node{source.bsp.at(static_cast<size_t>(reference))};
      auto const &surface{source.polygons.at(static_cast<size_t>(node.polygon))};
      auto const &normal{source.points.at(surface.normal)};
      int64_t dot{-static_cast<int64_t>(node.constant)};
      for(unsigned int axis{0}; axis != 3; ++axis) dot += static_cast<int64_t>(normal[axis]) * current.eye[axis];
      bool const front{wrap_long(dot) < 0};
      self(front ? node.front : node.back, depth + 1);
      polygon(source, surface, current);
      self(front ? node.back : node.front, depth + 1);
    }};
    walk(0, 0);
  }

  void billboard(part_header const &header, billboard_part const &source, part_view const &current) {
    if(source.bitmap >= textures.size()) throw std::out_of_range{"billboard bitmap exceeds bank"};
    auto const &texture{textures[source.bitmap]};
    auto point{rotate_vector(vec3<int32_t>{header.centre}, current.transform)};
    for(unsigned int axis{0}; axis != 3; ++axis) point[axis] = wrap_long(static_cast<int64_t>(point[axis]) + current.transform.translation[axis]);
    if(point.y <= view.projection.near_distance) return;
    auto const shift{view.projection.perspective_shift ? view.projection.perspective_shift : 8};
    auto const divisor{view.projection.perspective_shift ? point.y : view.projection.orthographic_divisor};
    auto const scale{wrap_long(static_cast<int64_t>(header.radius) * 4 * (int64_t{1} << shift)) / divisor};
    auto const scaled{[scale](int32_t const value){
      int64_t const product{static_cast<int64_t>(scale) * value};
      uint32_t const rounding{(product == static_cast<int32_t>(product) ? 0U : ~0U) ^ 128U};
      return wrap_long((product + rounding) >> 8);
    }};
    auto position{project_point(point, view.projection, projection_source::model)};
    position.x -= scaled(source.offset.x);
    position.y -= scaled(source.offset.y);
    vec2<int32_t> const size{scaled(static_cast<int32_t>(texture.width)), scaled(static_cast<int32_t>(texture.height))};
    if(size.x < 200 && size.y < 200) draw_scaled_bitmap(target, texture, vec2<int32_t>{position}, vec2<int32_t>{size}, view.bounds);
  }

public:
  model_draw(framebuffer &buffer, palette const &palette_data, texture_shade_maps const &shades, model_instance const &model, model_transform const &placement, model_view const &camera, std::span<bitmap const> const bank, std::span<light const> const sources, std::span<uint16_t const> const frames, std::span<model_attachment const> const replacements)
    : target{buffer}, colours{palette_data}, texture_shades{shades}, instance{model}, object{placement}, view{camera}, active_detail{camera.detail}, textures{bank}, lights{sources}, cells{frames}, attachments{replacements} {
  }

  void part(model_part const &source) {
    if(source.kind != model_kind::null) {
      auto const found{std::ranges::find(attachments, source.header.id, &model_attachment::body_part)};
      if(found != attachments.end()) {
        if(found->geometry){
          auto weapon_view{view};
          if(view.weapon_detail_bias)weapon_view.detail.bias=*view.weapon_detail_bias;
          model_draw{target, colours, texture_shades, instance, object, weapon_view, found->textures, lights, found->cells, {}}.part(*found->geometry);
          // The original weapon wrapper restores the global bias to zero,
          // rather than restoring the enclosing HERC's value.
          if(view.weapon_detail_bias)active_detail.bias=0;
        }
        return;
      }
    }
    auto const current{context(source.header.transform)};
    std::visit([&](auto const &value){
      using type = std::decay_t<decltype(value)>;
      if constexpr(std::is_same_v<type, model_mesh>) mesh(value, current);
      else if constexpr(std::is_same_v<type, bsp_part>) {
        for(auto const index : bsp_draw_order(value, current.eye, instance.transforms())) part(value.children.at(index));
      } else if constexpr(std::is_same_v<type, detail_part>) {
        auto detail{active_detail};
        detail.projection_shift = view.projection.perspective_shift;
        part(value.children.at(detail_draw_index(value, source.header.radius, current.eye, detail)));
      } else if constexpr(std::is_same_v<type, cell_part>) part(value.children.at(cell_draw_index(value, cells)));
      else if constexpr(std::is_same_v<type, part_list> || std::is_same_v<type, shape_part>) {
        for(auto const &child : value.children) part(child);
      } else if constexpr(std::is_same_v<type, billboard_part>) billboard(source.header, value, current);
    }, source.content);
  }
};

} // namespace

model_renderer::model_renderer(framebuffer &buffer, palette const &palette_data) : target{buffer}, colours{palette_data}, texture_shades{make_texture_shades(palette_data)} {
}

void model_renderer::draw(model_part const &source, model_instance const &instance, model_transform const &object, model_view const &view, std::span<bitmap const> const textures, std::span<light const> const lights, std::span<model_attachment const> const attachments) {
  model_draw{target, colours, texture_shades, instance, object, view, textures, lights, instance.cell_frames(), attachments}.part(source);
}

} // namespace earthsiege
