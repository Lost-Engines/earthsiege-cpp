#include "herc_body.h"
#include "simulation/model_math.h"

namespace earthsiege {

herc_body::herc_body(herc_definition const &definition, weapon_catalogue const &weapons, model_resource const &templates, std::span<int16_t const> const equipment, vec3<int32_t> const &position, vec3<uint16_t> const &angles)
  : source{definition}, rig{std::get<shape_part>(definition.models.roots.front().content)}, armament{definition.models.roots.front(), definition.mounts, equipment, weapons, templates}, movement_state{.position{vec3<int32_t>{position}}, .angles{vec3<uint16_t>{angles}}} {
  body_channel = rig.add_channel(source.profile.movement.forward_stop_sequence);
  torso_channel = rig.add_channel(source.profile.torso.sequence);
  elevation_channel = rig.add_channel(source.profile.elevation.sequence);
}

void herc_body::step(herc_controls const &controls, int16_t const frame_ratio) {
  update_player_movement(movement_state, source.profile.movement, rig.channel(body_channel), controls.throttle, controls.steering);
  update_aim(torso_state, source.profile.torso, controls.torso, frame_ratio);
  update_aim(elevation_state, source.profile.elevation, controls.elevation, frame_ratio);
  rig.channel(torso_channel).set_phase(source.profile.torso.sequence, static_cast<int16_t>(static_cast<uint16_t>(torso_state.angle) >> 2));
  rig.channel(elevation_channel).set_phase(source.profile.elevation.sequence, static_cast<int16_t>(static_cast<uint16_t>(elevation_state.angle) >> 2));
  advance_root_movement(movement_state, rig.channel(body_channel), frame_ratio);
  rig.update_pose();
}

locomotion_state const &herc_body::movement() const {
  return movement_state;
}

aiming_state const &herc_body::torso() const {
  return torso_state;
}

aiming_state const &herc_body::elevation() const {
  return elevation_state;
}

model_instance const &herc_body::model() const {
  return rig.instance();
}

weapon_assembly const &herc_body::weapons() const {
  return armament;
}

model_transform herc_body::placement() const {
  auto result{rotation_transform(movement_state.angles)};
  result.translation = movement_state.position;
  return result;
}

} // namespace earthsiege
