#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include "resources/resource_store.h"
#include "resources/world.h"
#include "simulation/model_math.h"
#include "simulation/scenery_world.h"

namespace {

void require(bool const success, std::string const &name, std::string const &field) {
  if(!success) throw std::runtime_error{name + ": " + field + " differs"};
}

} // anonymous namespace

int main(int const argc, char **argv) try {
  if(argc != 3) return EXIT_FAILURE;
  earthsiege::resource_store resources{argv[1]};
  auto const bytes{earthsiege::read_file(argv[2])};
  earthsiege::binary_reader file{bytes};
  auto const magic{file.take(8)};
  require(std::string(magic.begin(), magic.end()) == "ESWORLD1", "world", "fixture version");
  auto const zones{file.integer<uint32_t>()};
  unsigned int placements{0}, loaded_zones{0};
  for(unsigned int row{0}; row != zones; ++row) {
    auto const name{file.cstring()};
    auto const split{name.find('/')};
    auto const volume{name.substr(0, split)};
    auto const path{name.substr(split + 1)};
    auto const terrain{file.integer<uint16_t>()};
    auto const zone{earthsiege::read_zone(resources.read(volume, path))};
    require(zone.stored_terrain == terrain, name, "stored terrain");
    auto const metadata_bytes{file.take(file.integer<uint16_t>())};
    auto const metadata{earthsiege::read_terrain_metadata(resources.read("simvol0", "dat/" + std::string{earthsiege::terrain_name(static_cast<earthsiege::terrain_kind>(terrain))} + ".DAT"))};
    earthsiege::binary_reader expected_metadata{metadata_bytes};
    for(auto const &entry : metadata) {
      for(auto const &endpoint : entry.sort_endpoints) {
        for(unsigned int axis{0}; axis != 3; ++axis) require(endpoint[axis] == expected_metadata.integer<int32_t>(), name, "sort endpoint");
      }
    }
    expected_metadata.require_end();
    auto const count{file.integer<uint16_t>()};
    require(zone.placements.size() == count, name, "placement count");
    for(auto const &item : zone.placements) {
      require(item.shape == file.integer<uint16_t>(), name, "shape");
      for(unsigned int axis{0}; axis != 3; ++axis) require(item.position[axis] == file.integer<int32_t>(), name, "position");
      for(unsigned int axis{0}; axis != 3; ++axis) require(item.angles[axis] == file.integer<uint16_t>(), name, "angles");
      require(metadata.at(item.shape).used_by_instance() == (file.integer<uint8_t>() != 0), name, "metadata binding");
      ++placements;
    }
    /// Every selected zone also constructs owned geometry, palette, textures and colliders
    bool const shadowed{volume == "zones" && resources.archives().at("simvol0").find(path)};
    if(shadowed) continue;
    auto const zone_number{std::stoi(path.substr(path.find_last_of('/') + 5))};
    earthsiege::mission_header header{.terrain{static_cast<earthsiege::terrain_kind>(terrain)}, .zone{static_cast<int16_t>(zone_number)}};
    earthsiege::scenery_world world{resources, header};
    require(world.objects().size() == count && world.colliders().size() == count, name, "owned instance count");
    for(size_t index{0}; index != count; ++index) {
      auto const &actual{world.objects()[index]};
      auto const &expected{zone.placements[index]};
      require(actual.shape == expected.shape && actual.placement.translation == expected.position, name, "owned placement");
      require(actual.placement.basis == earthsiege::rotation_transform(expected.angles).basis, name, "owned rotation");
      require(bool(actual.metadata) == metadata.at(expected.shape).used_by_instance(), name, "owned metadata");
      auto const &root{world.terrain().roots.at(expected.shape)};
      auto const &field{std::get<earthsiege::shape_part>(root.content).collision};
      require(world.colliders()[index].field == (field ? &*field : nullptr) && world.colliders()[index].radius == root.header.radius, name, "owned collider");
    }
    ++loaded_zones;
  }
  auto const environments{file.integer<uint16_t>()};
  for(unsigned int index{0}; index != environments; ++index) {
    auto const name{file.cstring()};
    auto const environment{earthsiege::read_world_environment(resources.read("simvol0", name.substr(name.find('/') + 1)))};
    for(auto const value : environment.sky_ground_parameters) require(value == file.integer<int16_t>(), name, "sky/ground");
    for(auto const &band : environment.distance_bands) {
      require(band.size() == file.integer<uint32_t>(), name, "distance count");
      for(auto const value : band) require(value == file.integer<int32_t>(), name, "distance");
    }
    require(environment.shade_levels == file.integer<int16_t>(), name, "shade levels");
    for(std::span<earthsiege::palette_range const> const ranges : {std::span<earthsiege::palette_range const>{environment.near_ranges}, std::span<earthsiege::palette_range const>{environment.far_ranges}, std::span<earthsiege::palette_range const>{environment.blend_ranges}}) {
      require(ranges.size() == file.integer<uint16_t>(), name, "palette range count");
      for(auto const &range : ranges) {
        require(range.first == file.integer<int16_t>(), name, "palette first");
        require(range.count == file.integer<int16_t>(), name, "palette count");
      }
    }
    require(environment.stored_far_range_count == file.integer<int16_t>(), name, "stored range count");
    for(auto const value : environment.effect_parameters) require(value == file.integer<int16_t>(), name, "effect parameter");
    for(auto const value : environment.model_distance_offsets) require(value == file.integer<int32_t>(), name, "model distance offset");
    for(auto const &value : environment.resource_names) require(value == file.cstring(), name, "resource name");
    earthsiege::mission_header header{.terrain{static_cast<earthsiege::terrain_kind>(index / 6)}, .zone{-1}, .world_variant{static_cast<int16_t>(index % 2)}, .palette_variant{static_cast<int16_t>((index % 6) / 2)}};
    auto const selection{earthsiege::select_world(header)};
    require(selection.environment == static_cast<int>(index) && selection.palette == static_cast<int>(index - index % 2), name, "mission selection");
    earthsiege::scenery_world world{resources, header};
    require(world.objects().empty() && world.environment().resource_names == environment.resource_names, name, "negative zone and environment selection");
  }
  file.require_end();
  require(zones == 106 && placements == 5129 && loaded_zones == 98 && environments == 24, "coverage", "counts");
  std::cout << zones << " original zones, " << placements << " placements, " << loaded_zones << " owned scenery worlds and all " << environments << " environment descriptors agree" << std::endl;
  return EXIT_SUCCESS;
} catch(std::exception const &error) {
  std::cerr << error.what() << std::endl;
  return EXIT_FAILURE;
}
