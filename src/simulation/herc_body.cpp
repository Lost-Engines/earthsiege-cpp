#include "herc_body.h"
#include <algorithm>
#include <optional>
#include <stdexcept>
#include <utility>
#include "resources/model_tree.h"
#include "simulation/component_damage.h"
#include "simulation/fixed_math.h"
#include "simulation/model_math.h"

namespace earthsiege {

herc_body::herc_body(herc_definition const &definition, weapon_catalogue const &weapons, model_resource const &templates, std::span<int16_t const> const equipment, vec3<int32_t> const &position, vec3<uint16_t> const &angles)
  : source{definition}, rig{std::get<shape_part>(definition.models.roots.front().content)}, armament{definition.models.roots.front(), definition.mounts, equipment, weapons, templates}, movement_state{.position{vec3<int32_t>{position}}, .angles{vec3<uint16_t>{angles}}} {
  body_channel = rig.add_channel(source.profile.movement.forward_stop_sequence);
  torso_channel = rig.add_channel(source.profile.torso.sequence);
  elevation_channel = rig.add_channel(source.profile.elevation.sequence);
}

void herc_body::apply_damage(damage_consequences const &consequences) {
  damage = {consequences.disabled, consequences.movement_light, consequences.movement_heavy};
}

void herc_body::apply_damage(damage_state const &state) {
  auto cells{rig.instance().cell_frames()};
  if(cells.size() != state.cells.size()) throw std::invalid_argument{"HERC damage cells do not match its model"};
  for(size_t i{0}; i != cells.size(); ++i) cells[i] = static_cast<uint16_t>(state.cells[i]);
  armament.disable_missing_sections(state.present);
  apply_damage(state.consequences);
}

void herc_body::recover_from_collision(int32_t const duration, bool const reverse) {
  movement_state.collision_recovery = duration;
  movement_state.reverse_recovery = reverse;
}

std::span<movement_event const> herc_body::movement_events() const { return frame_movement_events; }

bool herc_body::step(herc_controls const &controls, int16_t const time_ratio, std::function<bool(herc_body const &)> const &contact, movement_event_sink const &events) {
  frame_movement_events.clear();
  auto const notify{[&](movement_event const event){ frame_movement_events.push_back(event); if(events) events(event); }};
  update_player_movement(movement_state, source.profile.movement, rig.channel(body_channel), controls.throttle, controls.steering, damage, time_ratio, notify);
  return advance_body(vec2<int16_t>{controls.torso, controls.elevation}, time_ratio, contact);
}

bool herc_body::step_requested(int16_t const speed, int16_t const steering, vec2<int16_t> const &aim, int16_t const time_ratio, std::function<bool(herc_body const &)> const &contact, movement_event_sink const &events) {
  frame_movement_events.clear();
  auto const notify{[&](movement_event const event){ frame_movement_events.push_back(event); if(events) events(event); }};
  update_requested_movement(movement_state, source.profile.movement, rig.channel(body_channel), speed, steering, damage, time_ratio, notify);
  return advance_body(aim, time_ratio, contact);
}

bool herc_body::advance_body(vec2<int16_t> const &aim, int16_t const time_ratio, std::function<bool(herc_body const &)> const &contact) {
  update_aim(torso_state, source.profile.torso, aim.x, time_ratio);
  update_aim(elevation_state, source.profile.elevation, aim.y, time_ratio);
  rig.channel(torso_channel).set_phase(source.profile.torso.sequence, static_cast<int16_t>(static_cast<uint16_t>(torso_state.angle) >> 2));
  rig.channel(elevation_channel).set_phase(source.profile.elevation.sequence, static_cast<int16_t>(static_cast<uint16_t>(elevation_state.angle) >> 2));
  std::optional<animation_rig> saved;
  if(contact) saved.emplace(rig);
  auto const position{movement_state.position};
  auto const advance{[&]{
    advance_root_movement(movement_state, rig.channel(body_channel), time_ratio);
    rig.update_pose();
  }};
  auto const restore{[&]{
    movement_state.position = position;                                        // original rollback restores the matrix/position, retaining Euler angles
    rig.restore(*saved);
  }};
  advance();
  if(!contact || !contact(*this)) return false;
  restore();
  movement_state.throttle = 0;
  auto &channel{rig.channel(body_channel)};
  if((std::to_underlying(channel.state().flags) & std::to_underlying(animation_channel_flags::transition)) == 0) {
    movement_state.gait_rate = std::clamp(wrap_word(-movement_state.gait_rate), source.profile.movement.reverse_fast_exit_rate, source.profile.movement.forward_rate);
    movement_state.playback_rate = wrap_word(-movement_state.playback_rate);
    advance();
    if(contact(*this)) {
      restore();
      movement_state.gait_rate = 0;
    }
  } else channel.reset(source.profile.movement.forward_stop_sequence);
  return true;
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

contact_actor herc_body::contact_geometry() const {
  auto const part{find_model_part(source.models.roots.front(), source.profile.camera_part)};
  int32_t const height{part && part->header.transform >= 0 ? rig.instance().transforms()[static_cast<size_t>(part->header.transform)].translation.z : 0};
  return {
    .position{vec3<int32_t>{movement_state.position}}, .angles{vec3<uint16_t>{movement_state.angles}},
    .radius{source.profile.collision_radius}, .gait_rate{movement_state.gait_rate},
    .mass_factor{source.profile.mass_factor}, .impact_height{height},
  };
}

} // namespace earthsiege
