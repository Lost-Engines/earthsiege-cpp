#include "actor_placement.h"
#include <map>
#include <set>
#include <span>
#include <stdexcept>
#include <utility>
#include "resources/name_table.h"
#include "resources/resource_store.h"
#include "simulation/fixed_math.h"
#include "simulation/model_math.h"

namespace earthsiege {
namespace {

size_t kind_index(actor_kind const kind) {
  auto const index{static_cast<size_t>(kind)};
  if(index >= 3) throw std::out_of_range{"unknown actor kind"};
  return index;
}

uint16_t route_heading(vec3<int32_t> const &first, vec3<int32_t> const &second) {
  auto x{wrap_long(static_cast<int64_t>(second.x) - first.x)};
  auto const y{wrap_long(static_cast<int64_t>(second.y) - first.y)};
  if(x == 0 && y == 0) x = 1;
  return static_cast<uint16_t>(direction_angle(x, y) - angle_quarter_turn);
}

} // anonymous namespace

placement_catalogue::placement_catalogue(resource_store const &resources) {
  for(auto const &name : read_resource_names(resources.read("simvol0", "nam/MECHS.NAM"))) {
    hercs.push_back(chassis{.name{name}, .profile{read_herc_profile(resources.read("simvol0", "dat/" + name + ".DAT"))}, .mounts{read_weapon_mounts(resources.read("simvol0", "gl/" + name + ".GL"))}});
  }
  std::array<std::string_view, 3> const files{"MFORMS", "FFORMS", "BFORMS"};
  for(size_t kind{0}; kind != formations.size(); ++kind) {
    binary_reader file{resources.read("simvol0", "dat/" + std::string{files[kind]} + ".DAT"), "formations"};
    auto const count{file.integer<uint16_t>()};
    auto const members{kind == 1 ? 3 : 7};
    for(unsigned int index{0}; index != count; ++index) {
      std::vector<vec3<int32_t>> offsets;
      for(int member{0}; member != members; ++member) {
        if(kind == 2) offsets.push_back(vec3<int32_t>{file.integer<int32_t>(), file.integer<int32_t>(), 0});
        else if(kind == 1) offsets.push_back(vec3<int32_t>{file.integer<int16_t>(), file.integer<int16_t>(), file.integer<int16_t>()});
        else offsets.push_back(vec3<int32_t>{file.integer<int16_t>(), file.integer<int16_t>(), 0});
      }
      formations[kind].push_back(std::move(offsets));
    }
    file.require_end();
  }
}

mission_placement placement_catalogue::place(mission_script const &script, player_roster const &player) const {
  if(script.groups.empty()) throw std::runtime_error{"mission has no player group"};
  mission_placement result;
  std::array<std::set<size_t>, 3> references;
  for(size_t index{1}; index != script.groups.size(); ++index) {
    auto const &group{script.groups[index]};
    for(auto const member : group.members) if(member >= 0) references[kind_index(group.kind)].insert(static_cast<size_t>(member));
  }
  for(size_t index{0}; index != player.hercs.size(); ++index) references[0].insert(script.hercs.size() + index);
  std::array<std::map<size_t, size_t>, 3> slots;
  for(size_t kind{0}; kind != references.size(); ++kind) {
    for(auto const source : references[kind]) {
      placed_actor actor{.source{source}, .definition{0}, .position{vec3<int32_t>{0, 0, -1}}, .angles{32768, 32768, 0}, .weapons{}};
      int16_t position{-1}, heading{-1};
      if(kind == 0) {
        actor.player_roster = source >= script.hercs.size();
        std::span<int16_t const> weapons;
        if(actor.player_roster) {
          auto const index{source - script.hercs.size()};
          auto const &entry{player.hercs.at(index)};
          actor.definition = entry.chassis;
          weapons = entry.weapons;
          actor.player_controlled = index == static_cast<size_t>(player.selected);
        } else {
          auto const &entry{script.hercs.at(source)};
          actor.definition = entry.chassis;
          position = entry.position;
          heading = entry.heading;
          weapons = entry.weapons;
        }
        auto const &definition{hercs.at(static_cast<size_t>(actor.definition))};
        actor.height = definition.profile.position_height;
        for(auto const &mount : definition.mounts) {
          if(mount.equipment_slot < 0 || static_cast<size_t>(mount.equipment_slot) >= weapons.size()) throw std::out_of_range{"mount equipment slot exceeds roster"};
          actor.weapons.push_back(weapons[static_cast<size_t>(mount.equipment_slot)]);
        }
      } else if(kind == 1) {
        auto const &entry{script.flyers.at(source)};
        actor.definition = entry.type;
        position = entry.position;
        heading = entry.heading;
      } else {
        auto const &entry{script.bases.at(source)};
        actor.definition = entry.type;
        position = entry.position;
        heading = entry.heading;
      }
      if(position >= 0) {
        actor.position = script.points.at(static_cast<size_t>(position));
        actor.position->z = wrap_long(static_cast<int64_t>(actor.position->z) + actor.height);
      }
      if(heading >= 0) actor.angles = vec3<uint16_t>{0, 0, decode_mission_heading(script.headings.at(static_cast<size_t>(heading)))};
      auto const slot{result.actors[kind].size()};
      slots[kind][source] = slot;
      if(actor.player_controlled) result.player = {.kind{actor_kind::herc}, .slot{slot}};
      result.actors[kind].push_back(std::move(actor));
    }
  }
  for(size_t index{0}; index != script.groups.size(); ++index) {
    auto const &source{script.groups[index]};
    auto const kind{kind_index(source.kind)};
    auto members{source.members};
    if(index == 0) for(size_t ordinal{0}; ordinal != members.size(); ++ordinal) members[ordinal] = ordinal < player.hercs.size() ? static_cast<int16_t>(script.hercs.size() + ordinal) : int16_t{-1};
    std::span<mission_reference const> route;
    if(source.route >= 0) route = script.routes.at(static_cast<size_t>(source.route)).points;
    uint16_t heading{0};
    if(source.heading >= 0) heading = decode_mission_heading(script.headings.at(static_cast<size_t>(source.heading)));
    else if(route.size() > 1) heading = route_heading(script.points.at(static_cast<size_t>(route[0])), script.points.at(static_cast<size_t>(route[1])));
    vec3<int32_t> const *anchor{nullptr};
    if(source.position >= 0) anchor = &script.points.at(static_cast<size_t>(source.position));
    else if(!route.empty()) anchor = &script.points.at(static_cast<size_t>(route[0]));
    placed_group group{.kind{source.kind}, .formation{source.formation}, .argument{index == 0 && source.argument == 11 ? int16_t{8} : source.argument}, .heading{heading}, .members{}, .mode{static_cast<uint8_t>(source.mode)}, .target{source.target_type}};
    for(size_t ordinal{0}; ordinal != members.size(); ++ordinal) {
      if(members[ordinal] < 0) continue;
      auto const slot{slots[kind].at(static_cast<size_t>(members[ordinal]))};
      auto &actor{result.actors[kind][slot]};
      actor.group = index;
      actor.ordinal = ordinal;
      if(actor.angles.x == 32768) actor.angles = vec3<uint16_t>{0, 0, heading};
      if(actor.position && actor.position->z < 0) {
        if(!anchor) throw std::runtime_error{"unpositioned actor has no group anchor"};
        if(ordinal != 0 && source.formation < 0) {
          /// The original reads before its formation allocation; no stable authored position exists
          actor.position.reset();
        } else {
          vec3<int32_t> offset;
          if(ordinal != 0) offset = formations[kind].at(static_cast<size_t>(source.formation)).at(ordinal - 1);
          auto const leader_angle{group.members.empty() ? actor.angles.z : result.actors[kind][group.members.front().slot].angles.z};
          auto const rotated{rotate_vector(offset, rotation_transform(vec3<uint16_t>{0, 0, leader_angle}))};
          actor.position = *anchor;
          for(unsigned int axis{0}; axis != 3; ++axis) (*actor.position)[axis] = wrap_long(static_cast<int64_t>((*actor.position)[axis]) + rotated[axis]);
          actor.position->z = wrap_long(static_cast<int64_t>(actor.position->z) + actor.height);
        }
      }
      if(kind == 1 && actor.position && actor.position->z == 0) actor.position->z = 5000;
      group.members.push_back(actor_reference{.kind{source.kind}, .slot{slot}});
    }
    result.groups.push_back(std::move(group));
  }
  return result;
}

} // namespace earthsiege
