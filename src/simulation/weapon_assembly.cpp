#include "weapon_assembly.h"
#include <stdexcept>
#include <utility>
#include "resources/model_tree.h"
#include "simulation/fixed_math.h"

namespace earthsiege {
namespace {

void bind_parent(model_part &part, int16_t const parent) {
  if(part.kind == model_kind::animated_shape) return;
  if(part.kind == model_kind::group || part.kind == model_kind::cell_animation) part.header.transform = parent;
  if(auto *bsp{std::get_if<bsp_part>(&part.content)}) for(auto &node : bsp->nodes) node.transform = parent;
  for(auto &child : model_children(part)) bind_parent(child, parent);
}

void shift_geometry(model_part &part, vec3<int16_t> const &offset) {
  if(part.kind == model_kind::animated_shape) return;
  if(auto *mesh{std::get_if<model_mesh>(&part.content)}) {
    std::vector<bool> used(mesh->points.size(), false);
    for(auto const &surface : mesh->polygons) {
      switch(surface.kind) {
        case model_kind::helper_polygon:
        case model_kind::solid_polygon:
        case model_kind::shaded_polygon:
        case model_kind::gouraud_polygon:
        case model_kind::textured_polygon: break;
        default: continue;
      }
      used.at(surface.centre) = true;
      for(unsigned int index{0}; index != surface.vertex_count; ++index) {
        auto const reference{surface.vertex_list + index};
        used.at(mesh->indices.empty() ? reference : mesh->indices.at(reference)) = true;
      }
    }
    for(size_t index{0}; index != used.size(); ++index) {
      if(!used[index]) continue;
      for(unsigned int axis{0}; axis != 3; ++axis) mesh->points[index][axis] = wrap_word(mesh->points[index][axis] + offset[axis]);
    }
  }
  for(auto &child : model_children(part)) shift_geometry(child, offset);
}

} // anonymous namespace

weapon_assembly::weapon_assembly(model_part const &owner, std::span<weapon_mount const> const mounts, std::span<int16_t const> const equipment, weapon_catalogue const &definitions, model_resource const &templates) {
  models.reserve(mounts.size());
  for(auto const &mount : mounts) {
    if(!mount.external()) continue;
    if(mount.equipment_slot < 0 || static_cast<size_t>(mount.equipment_slot) >= equipment.size()) throw std::out_of_range{"weapon mount equipment slot"};
    auto const *body{find_model_part(owner, mount.model_part)};
    if(!body) throw std::runtime_error{"weapon mount has no body part"};
    auto const selected{equipment[static_cast<size_t>(mount.equipment_slot)]};
    mounted_weapon_model weapon{.body_part{mount.model_part}, .weapon{selected}, .geometry{std::nullopt}, .cell_frames{}};
    if(selected != empty_weapon_slot) {
      auto const &definition{definitions.weapons.at(static_cast<size_t>(selected))};
      auto const model{definition.model_indices.at(mount.orientation)};
      if(model < 0) throw std::out_of_range{"external weapon has no model"};
      weapon.geometry = clone_model_part(templates.roots.at(static_cast<size_t>(model)));
      auto const offset{weapon_mount_offset(mount, definition)};
      shift_geometry(*weapon.geometry, offset);
      bind_parent(*weapon.geometry, body->header.transform);
      auto const &shape{std::get<shape_part>(weapon.geometry->content)};
      weapon.cell_frames.resize(shape.cell_frame_counts.size(), 0);
    }
    models.push_back(std::move(weapon));
  }
}

std::span<mounted_weapon_model const> weapon_assembly::parts() const {
  return models;
}

} // namespace earthsiege
