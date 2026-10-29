#include "mission_session.h"
#include "simulation/fixed_math.h"

namespace earthsiege {
size_t mission_session::player_id() const { return player_index; }

damage_display_state mission_session::instrument_damage(std::optional<size_t> const selected) const {
  damage_display_state result;
  if(!selected) return result;
  auto const index{*selected};
  result.friendly = activity->group(actor_group(index)).side == 0;
  result.player = index == player_index;
  if(index < crew.size()) {
    auto const &member{crew[index]};
    auto const &actor{*member.actor};
    result.kind = damage_display_kind::herc;
    for(auto const &[id, definition] : definitions) if(definition.get() == member.definition) { result.model = id; break; }
    result.dead = actor.damage().consequences.dead;
    result.shield_warning = actor.impacts().shield_warning;
    result.total_damage = actor.total_damage_fraction();
    auto const &damage{actor.damage()};
    auto const &capacities{actor.definition().damage};
    for(size_t i{}; i < result.armour.size(); ++i)
      result.armour[i] = damage.armour.at(i) < 0 ? int16_t{256} : wrap_word(static_cast<int32_t>(damage.armour[i]) * 256 / capacities.components.at(i).armour);
    for(size_t i{}; i < result.internal.size(); ++i)
      result.internal[i] = capacities.hit_points.at(i) == 0 ? int16_t{0} : actor.internal_damage_fraction(i);
  } else if(index < base_id(0)) {
    auto const &actor{*flyers.at(index - flyer_id(0)).actor};
    result.kind = damage_display_kind::flyer;
    result.total_damage = actor.total_damage_fraction();
  } else {
    auto const &actor{*bases.at(index - base_id(0)).actor};
    result.kind = damage_display_kind::ground;
    result.model = actor.profile().type;
    result.total_damage = wrap_word(static_cast<int32_t>(actor.damage()) * 256 / actor.profile().damage_capacity);
  }
  return result;
}

scanner_display_state mission_session::scanner_state(uint16_t const heading, uint32_t const ticks, int32_t const range) const {
  auto const &position{player().movement().position};
  scanner_display_state result{{position.x, position.y}, heading, ticks, range, player_actor().impacts().detectable, {}};
  auto const observations{observe()};
  for(size_t i{}; i < observations.size(); ++i) {
    if(i == player_index) continue;
    auto const &actor{observations[i]};
    auto const &identity{activity->actor(i)};
    // Original actors start with +0xa1 set. The last-position path in the
    // scanner is dormant until a producer clears it; perception does not.
    result.contacts.push_back({combat_kind(identity.kind), activity->group(identity.group).side,
      {actor.position.x, actor.position.y}, {0, 0}, !activity->waiting(i) && !actor.dead && !actor.disabled,
      identity.awareness.seen, true, displayed_target == i});
  }
  return result;
}

bool mission_session::camera_ready() const {
  for(auto const &slot : player_actor().weapons().slots())
    if(slot && slot->projectile && weapon_definitions.projectiles.at(*slot->projectile).kind == projectile_kind::rocket)
      if(auto const supply{slot->ammunition()}; supply && supply->rounds > 0) return true;
  return false;
}
bool mission_session::eo_selected() const {
  auto const selected{weapon_selection()};
  if(!selected) return false;
  auto const &slot{player_actor().weapons().slots()[*selected]};
  if(!slot || !slot->projectile) return false;
  auto const &projectile{weapon_definitions.projectiles.at(*slot->projectile)};
  return projectile.kind == projectile_kind::rocket && projectile.subtype == 3;
}
player_rocket_launch const &mission_session::latest_player_rocket() const { return camera_rocket; }
} // namespace earthsiege
