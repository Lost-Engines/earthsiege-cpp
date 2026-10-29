#include "mission_session.h"
#include "simulation/fixed_math.h"
#include <algorithm>

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

tactical_map_state mission_session::tactical_state(uint32_t const ticks) const {
  tactical_map_state result;result.ticks=ticks;
  // The original globals begin at zero, so the mission rectangle also encloses
  // the world origin when every authored point lies on the same side of it.
  for(auto const &p:points) {
    result.mission_bounds.left=std::min(result.mission_bounds.left,p.x);
    result.mission_bounds.right=std::max(result.mission_bounds.right,p.x);
    result.mission_bounds.top=std::min(result.mission_bounds.top,p.y);
    result.mission_bounds.bottom=std::max(result.mission_bounds.bottom,p.y);
  }
  result.mission_bounds.left-=60000;result.mission_bounds.top-=60000;
  result.mission_bounds.right+=60000;result.mission_bounds.bottom+=60000;
  auto const &group{activity->group(crew[player_index].group)};
  if(group.route>=0) {
    auto const &route{routes.at(static_cast<size_t>(group.route)).points};
    for(size_t i{1};i<route.size() && i<=9;++i) {
      tactical_contact c;c.frame=static_cast<int16_t>(i-1);c.size=9;c.position=points.at(static_cast<size_t>(route[i]));result.contacts.push_back(c);
    }
  }
  auto const wingmates{wingmate_ids()};auto const actors{observe()};
  for(size_t i{};i<actors.size();++i) {
    auto const &a{actors[i]};auto const &identity{activity->actor(i)};tactical_contact c;
    c.actor=i;c.side=activity->group(identity.group).side;c.dead=a.dead;c.hidden=activity->waiting(i);
    c.detected=identity.awareness.seen;c.position=a.position;c.heading=a.heading;
    if(i<crew.size()) {
      c.kind=0;c.size=7;c.frame=i==player_index?18:c.side?9:54;
      auto const it{std::ranges::find(wingmates,i)};if(it!=wingmates.end())c.frame=static_cast<int16_t>(27+9*(it-wingmates.begin()));
    } else if(i<base_id(0)){c.kind=2;c.size=5;c.frame=63;}
    else {
      auto const type{bases.at(i-base_id(0)).actor->profile().type};bool const small{type==5 || type==6 || type==8};
      c.kind=1;c.size=small?7:13;c.frame=small?(c.side?73:75):(c.side?72:74);
    }
    result.contacts.push_back(c);
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
