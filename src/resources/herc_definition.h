#pragma once
#include <string_view>
#include <vector>
#include "resources/artwork.h"
#include "resources/herc_profile.h"
#include "resources/model.h"
#include "resources/weapon_mounts.h"

namespace earthsiege {

class resource_store;

struct herc_definition {
  herc_profile profile;
  model_resource models;
  std::vector<weapon_mount> mounts;
  std::vector<bitmap> textures;
};

/// Load runtime skeleton bindings, texture group and start/stop animation priorities
herc_definition read_herc_definition(resource_store const &resources, std::string_view name);

} // namespace earthsiege
