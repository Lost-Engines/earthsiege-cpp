#include "weapon_firing.h"
#include <algorithm>
#include <stdexcept>
#include <utility>
#include "resources/model_tree.h"
#include "simulation/fixed_math.h"
#include "simulation/model_math.h"

namespace earthsiege {
namespace {

vec3<int16_t> muzzle_point(weapon_mount const &mount, weapon_definition const &definition, int16_t const lateral) {
  auto offset{weapon_mount_offset(mount, definition)};
  offset.x = wrap_word(offset.x + lateral);
  offset.y = wrap_word(offset.y + definition.muzzle_offset.y);
  offset.z = wrap_word(offset.z + definition.muzzle_offset.z);
  return offset;
}

} // anonymous namespace

weapon_muzzle prepare_weapon_muzzle(weapon_mount const &mount, weapon_definition const &definition, model_part const &root, std::span<model_transform const> const transforms, model_transform const &placement, vec2<int16_t> const &aim) {
  auto const *part{find_model_part(root, mount.model_part)};
  if(!part || part->header.transform < 0 || static_cast<size_t>(part->header.transform) >= transforms.size()) throw std::out_of_range{"weapon muzzle model transform"};
  auto local{transforms[static_cast<size_t>(part->header.transform)]};
  if(mount.pitch_sequence < 0 || mount.yaw_sequence < 0) {
    auto const extra{rotation_transform(vec3<uint16_t>{static_cast<uint16_t>(mount.pitch_sequence < 0 ? aim.x : 0), 0, static_cast<uint16_t>(mount.yaw_sequence < 0 ? aim.y : 0)})};
    local = compose_transforms(extra, local);
  }
  auto const transform{compose_transforms(local, placement)};
  return {transform, transform_point(muzzle_point(mount, definition, definition.muzzle_offset.x), transform), transform_angles(transform)};
}

std::vector<weapon_shot> discharge_weapon(weapon_state &state, weapon_catalogue const &definitions, weapon_mount const &mount, weapon_muzzle const &muzzle, bool const free_ammunition) {
  std::vector<weapon_shot> shots;
  if(std::holds_alternative<ecm_weapon>(state.mechanism)) return shots;
  auto const &definition{definitions.weapons.at(static_cast<size_t>(state.definition))};
  auto &control{state.controls()};
  control.refire_remaining = definition.refire_delay;
  control.hud_changes |= std::to_underlying(weapon_hud_change::fired);
  control.refreshed_changes |= std::to_underlying(weapon_hud_change::fired);
  if(auto *grenade{std::get_if<grenade_weapon>(&state.mechanism)}) {
    if(!free_ammunition) grenade->supply.rounds = wrap_word(grenade->supply.rounds - 1);
    grenade->holding = false;
    return shots;                                                             // original 0x7b44 grenade creation is a no-op
  }
  if(!state.projectile) throw std::logic_error{"firing weapon has no projectile"};
  auto const &projectile{definitions.projectiles.at(*state.projectile)};
  auto const emit{[&](shot_kind const kind, int16_t const charge, weapon_muzzle shot){
    if(kind == shot_kind::beam) shot.transform.translation = shot.position;
    shots.push_back({kind, *state.projectile, charge, kind == shot_kind::beam ? definition.range : 0, shot});
  }};
  auto const full{definition.energy_thresholds[1]};
  if(auto *ammunition{state.ammunition()}) {
    if(!free_ammunition) {
      ammunition->rounds = wrap_word(ammunition->rounds - full);
      if(ammunition->rounds < 1) { ammunition->rounds = 0; ammunition->available = false; }
    }
    bool const rocket{projectile.kind == projectile_kind::rocket};
    if(!rocket && mount.external()) control.firing_animation = true;
    emit(rocket ? shot_kind::rocket : shot_kind::bullet, -1, muzzle);
  } else if(auto *energy{state.energy()}) {
    if(state.elf()) {
      energy->charge = wrap_word(energy->charge - full);
      emit(shot_kind::beam, capacitor_maximum, muzzle);
    } else {
      if(mount.external()) control.firing_animation = true;
      if(projectile.kind == projectile_kind::beam) {
        auto const charge{std::min(energy->charge, full)};
        energy->charge = wrap_word(energy->charge - charge);
        emit(shot_kind::beam, charge, muzzle);
      } else if(energy->charge < full) {
        emit(shot_kind::scaled, energy->charge, muzzle);
        if(definition.barrels == 3) {
          for(auto const lateral : {int16_t{0}, wrap_word(-definition.muzzle_offset.x)}) {
            auto extra{muzzle};
            extra.position = transform_point(muzzle_point(mount, definition, lateral), muzzle.transform);
            emit(shot_kind::scaled, energy->charge, extra);
          }
        }
        energy->charge = 0;
      } else {
        energy->charge = wrap_word(energy->charge - full);
        emit(shot_kind::bullet, -1, muzzle);
      }
    }
  }
  return shots;
}

} // namespace earthsiege
