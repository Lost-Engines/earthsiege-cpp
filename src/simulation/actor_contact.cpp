#include "actor_contact.h"
#include <algorithm>
#include <stdexcept>
#include "simulation/fixed_math.h"
#include "simulation/model_math.h"

namespace earthsiege {
namespace {

int32_t weighted_speed(int32_t const value, int16_t const weight) {
  return wrap_long((static_cast<int64_t>(value) * weight + 512) >> 10);
}

} // namespace

std::optional<actor_contact> find_actor_contact(std::span<contact_actor const> const actors, size_t const moving) {
  if(moving >= actors.size()) throw std::out_of_range{"moving contact actor"};
  auto const &a{actors[moving]};
  for(size_t index{0}; index != actors.size(); ++index) {
    auto const &b{actors[index]};
    if(index == moving || b.waiting || b.radius == 0) continue;
    vec3<int32_t> const delta{wrap_long(static_cast<int64_t>(b.position.x) - a.position.x), wrap_long(static_cast<int64_t>(b.position.y) - a.position.y), wrap_long(static_cast<int64_t>(b.position.z) - a.position.z)};
    if(fast_distance(delta) >= a.radius + b.radius) continue;
    actor_contact result{.other{index}, .position{0, 0, 0}};
    if(b.herc) {
      auto const other_world{rotate_vector(vec3<int32_t>{0, b.gait_rate, 0}, rotation_transform(b.angles))};
      auto const other_local{inverse_rotate_vector(other_world, rotation_transform(a.angles))};
      auto const relative{wrap_long(static_cast<int64_t>(wrap_word(weighted_speed(a.gait_rate, a.mass_factor))) - weighted_speed(other_local.y, b.mass_factor))};
      int16_t constexpr impact_scale{300};
      int32_t constexpr damaging_impact{200};
      auto const impact{weighted_speed(relative, impact_scale)};
      if(impact > damaging_impact) {
        result.damage = impact;
        result.position = vec3<int32_t>{
          wrap_long(static_cast<int64_t>(a.position.x) + b.position.x) >> 1,
          wrap_long(static_cast<int64_t>(a.position.y) + b.position.y) >> 1,
          std::min(a.impact_height, b.impact_height),                             // original contact uses the local articulated height without world Z
        };
      }
    }
    return result;
  }
  return std::nullopt;
}

} // namespace earthsiege
