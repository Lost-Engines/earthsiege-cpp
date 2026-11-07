#include "herc_fire_control.h"
#include <array>
#include <stdexcept>
#include "simulation/fixed_math.h"
#include "simulation/herc_weapon_selection.h"
#include "simulation/model_math.h"
#include "simulation/projectile_flight.h"

namespace earthsiege {
void aim_and_fire_herc(herc_fire_state &state, weapon_bank const &bank, weapon_catalogue const &definitions, vec3<int32_t> const &position, herc_fire_target const &target, herc_accuracy const &accuracy, random_state &random, herc_fire_callbacks const &callbacks) {
  auto const distance{fast_distance_2d({wrap_long(static_cast<int64_t>(target.point.x) - position.x), wrap_long(static_cast<int64_t>(target.point.y) - position.y)})};
  auto selected{state.retained_weapon};
  if(selected) {
    auto const &weapon{bank.slots()[*selected].value()};
    auto const &definition{definitions.weapons.at(static_cast<size_t>(weapon.definition))};
    if(!weapon.ready(definition) || !weapon_in_range(definition, distance)) selected.reset();
  }
  if(!selected) {
    if(state.skip_selection) state.skip_selection = false;
    else {
      selected = callbacks.select_weapon(distance, target.relative_facing);
      state.retained_weapon.reset();
    }
  }
  auto point{vec3<int32_t>{target.point}};
  if(!selected) { callbacks.aim(point); return; }
  auto const &weapon{bank.slots()[*selected].value()};
  if(!weapon.projectile) throw std::logic_error{"AI selected weapon without projectile"};
  auto const speed{definitions.projectiles.at(*weapon.projectile).speed};
  auto const target_speed{callbacks.target_speed()};
  if(speed > 0 && target_speed != 0) {
    // The original narrows distance/4 and the final lead to signed words.
    // Keep the intervening 32-bit multiplication wrap for fast distant targets.
    auto const product{wrap_long(static_cast<int64_t>(wrap_word(distance >> 2)) * target_speed * 4)};
    auto const lead{wrap_word(product / speed)};
    auto const offset{rotate_vector({0, lead, 0}, rotation_transform({0, 0, target.heading}))};
    point.x = wrap_long(static_cast<int64_t>(point.x) + offset.x);
    point.y = wrap_long(static_cast<int64_t>(point.y) + offset.y);
  }
  if(accuracy.enemy) {
    static constexpr std::array<int16_t, 4> error_by_difficulty{1000, 666, 333, 0};
    auto const spread{error_by_difficulty.at(static_cast<size_t>(accuracy.campaign ? 3 : accuracy.difficulty))};
    // Original errors are positive on all three axes, not centred around zero.
    if(spread) for(unsigned int axis{0}; axis != 3; ++axis) point[axis] = wrap_long(static_cast<int64_t>(point[axis]) + random.bounded(spread));
  }
  auto const errors{callbacks.aim(point)};
  if(weapon_lock_channel(weapon, definitions) || (errors.x >= -1000 && errors.x < 1000 && errors.z >= -1000 && errors.z < 1000)) {
    callbacks.fire(*selected);
    if(weapon.definition == 3 || (definitions.edition == game_edition::expansion && weapon.definition == 18)) state.retained_weapon = selected;
  }
}
} // namespace earthsiege
