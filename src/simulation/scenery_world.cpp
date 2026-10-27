#include "scenery_world.h"
#include <stdexcept>
#include <string>
#include "resources/resource_store.h"
#include "simulation/model_math.h"

namespace earthsiege {

scenery_world::scenery_world(resource_store const &resources, mission_header const &header) {
  auto const selection{select_world(header)};
  environment_data = read_world_environment(resources.read("simvol0", "wld/WORLD" + std::to_string(selection.environment) + ".WLD"));
  palette_data = read_palette(resources.read("simvol0", "dpl/WORLD" + std::to_string(selection.palette) + ".DPL"));
  textures_data = read_bitmaps(resources.read("simvol0", "dba/BASETEX.DBA"));
  if(header.zone < 0) return;
  auto const terrain_file{std::string{terrain_name(header.terrain)}};
  terrain_data = read_model(resources.read("simvol0", "dgs/" + terrain_file + ".DGS"));
  metadata_data = read_terrain_metadata(resources.read("simvol0", "dat/" + terrain_file + ".DAT"));
  if(metadata_data.size() != terrain_data.roots.size()) throw std::runtime_error{"terrain metadata does not match shape count"};
  auto const filename{"dat/ZONE" + std::to_string(header.zone) + ".DAT"};
  /// The original mounted-volume search visits SIMVOL0 before ZONES
  auto const volume{resources.archives().at("simvol0").find(filename) ? "simvol0" : "zones"};
  auto const zone{read_zone(resources.read(volume, filename))};
  instances.reserve(zone.placements.size());
  collisions.reserve(zone.placements.size());
  for(auto const &source : zone.placements) {
    auto const &root{terrain_data.roots.at(source.shape)};
    auto const &shape{std::get<shape_part>(root.content)};
    auto placement{rotation_transform(source.angles)};
    placement.translation = vec3<int32_t>{source.position};
    auto const &metadata{metadata_data.at(source.shape)};
    instances.push_back(scenery_object{
      .shape{source.shape},
      .placement{placement},
      .model{model_instance{shape}},
      .metadata{metadata.used_by_instance() ? &metadata : nullptr},
    });
    collisions.push_back(scenery_collider{.field{shape.collision ? &*shape.collision : nullptr}, .placement{placement}, .radius{root.header.radius}});
  }
}

world_environment const &scenery_world::environment() const {
  return environment_data;
}

palette const &scenery_world::colours() const {
  return palette_data;
}

std::span<bitmap const> scenery_world::textures() const {
  return textures_data;
}

model_resource const &scenery_world::terrain() const {
  return terrain_data;
}

std::span<scenery_object const> scenery_world::objects() const {
  return instances;
}

std::span<scenery_collider const> scenery_world::colliders() const {
  return collisions;
}

} // namespace earthsiege
