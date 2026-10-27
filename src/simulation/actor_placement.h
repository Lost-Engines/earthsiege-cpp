#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>
#include "resources/herc_profile.h"
#include "resources/mission_script.h"
#include "resources/player_roster.h"
#include "resources/weapon_mounts.h"

namespace earthsiege {

class resource_store;

struct actor_reference {
  actor_kind kind{actor_kind::herc};
  size_t slot{0};
};

struct placed_actor {
  size_t source{0};
  int16_t definition{0};
  std::optional<vec3<int32_t>> position;
  vec3<uint16_t> angles;
  size_t group{0};
  size_t ordinal{0};
  int16_t height{0};
  bool player_roster{false};
  bool player_controlled{false};
  std::vector<int16_t> weapons;
};

struct placed_group {
  actor_kind kind{actor_kind::herc};
  int16_t formation{0};
  int16_t argument{0};
  uint16_t heading{0};
  std::vector<actor_reference> members;
  uint8_t mode{0};
  target_kind target{target_kind::none};
};

struct mission_placement {
  std::array<std::vector<placed_actor>, 3> actors;
  std::vector<placed_group> groups;
  actor_reference player;
};

/// Resource definitions used by the original pre-tick actor placement pass
class placement_catalogue {
  struct chassis {
    std::string name;
    herc_profile profile;
    std::vector<weapon_mount> mounts;
  };
  std::vector<chassis> hercs;
  std::array<std::vector<std::vector<vec3<int32_t>>>, 3> formations;

public:
  explicit placement_catalogue(resource_store const &resources);
  mission_placement place(mission_script const &script, player_roster const &player) const;
};

} // namespace earthsiege
