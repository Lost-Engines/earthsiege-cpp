#pragma once
#include <map>
#include <memory>
#include <vector>
#include "render/scenery_renderer.h"
#include "resources/mission_script.h"
#include "resources/player_roster.h"
#include "simulation/camera.h"
#include "simulation/herc_body.h"
#include "simulation/scenery_world.h"

namespace earthsiege {

class resource_store;

/// Own mission terrain and the player roster while other actor consumers are integrated
class mission_session {
  struct crew_member {
    herc_definition const *definition;
    std::unique_ptr<herc_body> body;
    std::vector<model_attachment> attachments;
    bool contact_received{false};
  };
  scenery_world terrain;
  weapon_catalogue weapon_definitions;
  model_resource weapon_models;
  std::vector<bitmap> weapon_textures;
  std::map<int16_t, std::unique_ptr<herc_definition>> definitions;
  std::vector<crew_member> crew;
  size_t player_index{0};
  std::vector<actor_contact> frame_contacts;

public:
  mission_session(resource_store const &resources, mission_script const &script, player_roster const &roster);
  mission_session(mission_session const &) = delete;
  mission_session(mission_session &&) = delete;
  mission_session &operator=(mission_session const &) = delete;
  mission_session &operator=(mission_session &&) = delete;
  scenery_world const &scenery() const;
  herc_body const &player() const;
  void step(herc_controls const &controls, int16_t frame_ratio);
  std::span<actor_contact const> contacts() const;
  std::vector<scene_model> roster_models() const;
  camera_target view_target() const;
  camera_state make_camera(camera_mode mode) const;
};

} // namespace earthsiege
