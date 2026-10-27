#include "herc_definition.h"
#include <array>
#include <string>
#include "resources/model_bindings.h"
#include "resources/resource_store.h"

namespace earthsiege {

herc_definition read_herc_definition(resource_store const &resources, std::string_view const name) {
  herc_definition result{
    .profile{read_herc_profile(resources.read("simvol0", "dat/" + std::string{name} + ".DAT"))},
    .models{read_model(resources.read("simvol0", "dts/" + std::string{name} + ".DTS"))},
    .mounts{read_weapon_mounts(resources.read("simvol0", "gl/" + std::string{name} + ".GL"))},
    .textures{},
  };
  bind_herc_detail_nodes(result.models, result.profile.detail_part_ids);
  std::array<std::string_view, 4> constexpr texture_names{"LIGHT", "MEDIUM", "HEAVY", "ENEMY"};
  auto const texture{texture_names.at(static_cast<size_t>(result.profile.textures))};
  result.textures = read_bitmaps(resources.read("simvol0", "dba/" + std::string{texture} + ".DBA"));
  auto &animation{std::get<shape_part>(result.models.roots.front().content).animation.value()};
  for(auto const sequence : {result.profile.movement.forward_stop_sequence, result.profile.movement.reverse_stop_sequence}) animation.sequences.at(static_cast<size_t>(sequence)).priority = 0;
  return result;
}

} // namespace earthsiege
