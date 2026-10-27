#include "mission_session.h"
#include <optional>
#include <stdexcept>
#include <utility>
#include "resources/model_tree.h"
#include "resources/name_table.h"
#include "resources/resource_store.h"
#include "simulation/actor_placement.h"

namespace earthsiege {

mission_session::mission_session(resource_store const &resources, mission_script const &script, player_roster const &roster)
  : terrain{resources, script.header},
    weapon_definitions{read_weapon_catalogue(resources.read("simvol0", "dat/WEAPONS.DAT"))},
    weapon_models{read_model(resources.read("simvol0", "dts/MECHWPNS.DTS"))},
    weapon_textures{read_bitmaps(resources.read("simvol0", "dba/WPNTEX.DBA"))} {
  auto const placements{placement_catalogue{resources}.place(script, roster)};
  auto const names{read_resource_names(resources.read("simvol0", "nam/MECHS.NAM"))};
  std::optional<size_t> controlled;
  for(auto const &actor : placements.actors[static_cast<size_t>(actor_kind::herc)]) {
    if(!actor.player_roster) continue;
    if(!actor.position) throw std::runtime_error{"player roster has an undefined formation position"};
    auto &definition{definitions[actor.definition]};
    if(!definition) definition = std::make_unique<herc_definition>(read_herc_definition(resources, names.at(static_cast<size_t>(actor.definition))));
    auto const &loadout{roster.hercs.at(actor.source - script.hercs.size())};
    crew_member member{
      .definition{definition.get()},
      .body{std::make_unique<herc_body>(*definition, weapon_definitions, weapon_models, loadout.weapons, *actor.position, actor.angles)},
      .attachments{},
    };
    for(auto const &part : member.body->weapons().parts()) member.attachments.push_back({.body_part{part.body_part}, .geometry{part.geometry ? &*part.geometry : nullptr}, .cells{part.cell_frames}, .textures{weapon_textures}});
    if(actor.player_controlled) controlled = crew.size();
    crew.push_back(std::move(member));
  }
  if(!controlled) throw std::runtime_error{"mission has no controlled roster HERC"};
  player_index = *controlled;
}

scenery_world const &mission_session::scenery() const {
  return terrain;
}

herc_body const &mission_session::player() const {
  return *crew.at(player_index).body;
}

void mission_session::step(herc_controls const &controls, int16_t const frame_ratio) {
  frame_contacts.clear();
  crew.at(player_index).body->step(controls, frame_ratio, [&](herc_body const &candidate){
    std::vector<contact_actor> bodies;
    bodies.reserve(crew.size());
    for(auto const &member : crew) bodies.push_back(member.body->contact_geometry());
    if(auto const hit{find_actor_contact(bodies, player_index)}) {
      crew.at(hit->other).contact_received = true;
      frame_contacts.push_back(*hit);                                          // retain damage dispatch for the component-damage consumer
      return true;
    }
    return scenery_contact_height(terrain.colliders(), candidate.movement().position) != 0;
  });
}

std::span<actor_contact const> mission_session::contacts() const { return frame_contacts; }

std::vector<scene_model> mission_session::roster_models() const {
  std::vector<scene_model> result;
  result.reserve(crew.size());
  for(auto const &member : crew) result.push_back({
    .roots{member.definition->models.roots}, .instance{member.body->model()},
    .placement{member.body->placement()}, .native_class{7}, .textures{member.definition->textures},
    .attachments{member.attachments}, .detail_thresholds{herc_detail_thresholds}, .detail{},
  });
  return result;
}

camera_target mission_session::view_target() const {
  auto const &member{crew.at(player_index)};
  camera_target result{.angles{vec3<uint16_t>{member.body->movement().angles}}, .placement{member.body->placement()}, .attachment{std::nullopt}};
  auto const part{find_model_part(member.definition->models.roots.front(), member.definition->profile.camera_part)};
  if(part) {
    if(part->header.transform < 0) throw std::runtime_error{"HERC camera part has no skeletal transform"};
    result.attachment = member.body->model().transforms()[static_cast<size_t>(part->header.transform)];
  }
  return result;
}

camera_state mission_session::make_camera(camera_mode const mode) const {
  auto const &profile{crew.at(player_index).definition->profile};
  camera_state result;
  result.mode = mode;
  result.focus_offset = profile.camera_focus;
  result.attachment_offset = profile.camera_offset;
  update_camera(result, view_target());
  return result;
}

} // namespace earthsiege
