#include "ground_definition.h"
#include <stdexcept>
#include <utility>
#include "resources/resource_store.h"
#include "resources/model_tree.h"

namespace earthsiege {
namespace {

void repair_radar_bsp(model_part &part) {
  // Both radar exports reference transforms 2..4 in three distant detail
  // branches, but their animation has only transforms 0 and 1. DOS reads the
  // following allocation (animation-channel bytes) as matrices. These planes
  // divide the stationary building; retain their local-space plane instead.
  if(auto *bsp{std::get_if<bsp_part>(&part.content)}) for(auto &node : bsp->nodes) if(node.transform >= 2) node.transform = -1;
  for(auto &child : model_children(part)) repair_radar_bsp(child);
}

} // namespace

ground_catalogue::ground_catalogue(resource_store const &resources)
  : static_models{read_model(resources.read("simvol0", "dgs/BASES.DGS")), read_model(resources.read("simvol0", "dgs/BASES_E.DGS"))},
    animated_models{read_model(resources.read("simvol0", "dts/BASES_AN.DTS")), read_model(resources.read("simvol0", "dts/BASES_AE.DTS"))},
    wrecks{read_model(resources.read("simvol0", "dgs/BHULKS.DGS"))},
    texture_bank{read_bitmaps(resources.read("simvol0", "dba/BASETEX.DBA"))} {
  binary_reader file{resources.read("simvol0", "dat/BASES.DAT"), "base definitions"};
  binary_reader collisions{resources.read("simvol0", "dat/BASECOL.DAT"), "base collision"};
  auto const count{file.integer<uint16_t>()};
  for(unsigned int index{0}; index != count; ++index) {
    ground_profile p;
    for(auto &v : p.source) v = file.integer<int16_t>();
    auto const &v{p.source};
    p.type = v[0]; p.model = v[1]; p.wreck_model = v[2]; p.channels = v[3]; p.damage_capacity = v[4];
    p.destruction_centre = vec3<int16_t>{v[6], v[7], v[8]};
    p.burst_centre = vec3<int16_t>{v[9], v[10], v[11]}; p.burst_half_extent = vec3<int16_t>{v[12], v[13], v[14]};
    p.weak_point = vec3<int16_t>{v[15], v[16], v[17]}; p.weak_point_radius = v[18];
    p.animation_rates = {v[19], v[20]}; p.cell_sequence = v[21]; p.cell_period = v[22];
    p.contact_radius = v[24]; p.target_height = v[25]; p.can_retaliate = v[26] != 0;
    if(p.type != static_cast<int>(index) || p.channels < 0 || p.channels > 2) file.fail("unsupported base definition");
    p.collision = read_collision_parts(collisions);
    definitions.push_back(std::move(p));
  }
  file.require_end(); collisions.require_end();
  for(auto &bank : animated_models) repair_radar_bsp(bank.roots.at(0));
}

std::span<ground_profile const> ground_catalogue::profiles() const { return definitions; }
ground_profile const &ground_catalogue::profile(size_t const type) const { return definitions.at(type); }
model_part const &ground_catalogue::model(size_t const type, uint8_t const side, bool const destroyed) const {
  auto const &p{profile(type)};
  if(destroyed) return wrecks.roots.at(static_cast<size_t>(p.wreck_model));
  auto const bank{side == 0 ? 0U : 1U};
  return (p.channels == 0 ? static_models[bank] : animated_models[bank]).roots.at(static_cast<size_t>(p.model));
}
std::span<bitmap const> ground_catalogue::textures() const { return texture_bank; }

} // namespace earthsiege
