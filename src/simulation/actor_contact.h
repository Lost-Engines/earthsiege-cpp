#pragma once
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include "vectorstorm/vector/vector3.h"

namespace earthsiege {

struct contact_actor {
  vec3<int32_t> position;
  vec3<uint16_t> angles;
  int16_t radius{0};
  int16_t gait_rate{0};
  int16_t mass_factor{0};
  int32_t impact_height{0};
  bool herc{true};
  bool waiting{false};
};

struct actor_contact {
  size_t other{0};
  int32_t damage{0};
  vec3<int32_t> position;
};

inline int16_t constexpr ramming_damage_radius{1200};

/// First active overlapping body wins; positive damage applies to both bodies
std::optional<actor_contact> find_actor_contact(std::span<contact_actor const> actors, size_t moving);

} // namespace earthsiege
