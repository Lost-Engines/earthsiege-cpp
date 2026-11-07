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

herc_body::herc_body(herc_definition const &definition, weapon_catalogue const &weapons, model_resource const &templates, std::span<int16_t const> const equipment, vec3<int32_t> const &position, vec3<uint16_t> const &angles, sound_sink sink)
  : source{definition}, sounds{std::move(sink)}, rig{std::get<shape_part>(definition.models.roots.front().content)}, armament{definition.models.roots.front(), definition.mounts, equipment, weapons, templates}, movement_state{.position{vec3<int32_t>{position}}, .angles{vec3<uint16_t>{angles}}} {
  expansion = weapons.edition == game_edition::expansion;
  body_channel = rig.add_channel(source.profile.movement.forward_stop_sequence);
  if(source.profile.torso.sequence >= 0) torso_channel = rig.add_channel(source.profile.torso.sequence);
  if(source.profile.elevation.sequence >= 0) elevation_channel = rig.add_channel(source.profile.elevation.sequence);
  // Equipping the original actor evaluates all channels at time zero before
  // binding weapon geometry. Some enemy chassis differ from the raw defaults.
  rig.update_pose();
  for(size_t i{};i<ground.size()&&source.profile.attachment_types[i]>=0;++i){
    ground[i].shape=source.profile.attachment_types[i];ground[i].active=true;
  }
}

void herc_body::place(vec3<int32_t> const &position, vec3<uint16_t> const &angles) {
  movement_state.position = position;
  movement_state.angles = angles;
}

void herc_body::mark_collapsed() { movement_state.collapsed = true; }

void herc_body::apply_damage(damage_consequences const &consequences) {
  damage = {consequences.disabled, consequences.movement_light, consequences.movement_heavy};
}

void herc_body::apply_damage(damage_state const &state) {
  auto cells{rig.instance().cell_frames()};
  if(cells.size() != state.cells.size()) throw std::invalid_argument{"HERC damage cells do not match its model"};
  for(size_t i{0}; i != cells.size(); ++i) cells[i] = static_cast<uint16_t>(state.cells[i]);
  armament.disable_missing_sections(state.present);
  apply_damage(state.consequences);
  // 13250 permanently removes each destroyed leg's flat attachment.
  for(int i{};i<source.profile.legs;++i){
    auto const part{static_cast<size_t>(i<2?i:i+8)};
    auto const amount{state.internal.at(part)};
    auto const capacity{source.damage.hit_points.at(part)};
    if(amount<0||(capacity&&amount*256/capacity==256))ground.at(static_cast<size_t>(i)).active=false;
  }
  if(state.consequences.dead&&source.profile.remove_when_destroyed)
    for(auto &attachment:ground)attachment.active=false;
}

void herc_body::update_weapons(weapon_bank const &bank) { armament.update_cells(bank); }

void herc_body::recover_from_collision(int32_t const duration, bool const reverse) {
  movement_state.collision_recovery = duration;
  movement_state.reverse_recovery = reverse;
}

std::span<movement_event const> herc_body::movement_events() const { return frame_movement_events; }

void herc_body::begin_frame() { frame_movement_events.clear(); }

void herc_body::prepare_player(herc_controls const &controls, frame_ratio const ratio, movement_event_sink const &events) {
  prepare_player_movement(controls.throttle, controls.steering, ratio, events);
  prepare_aim({controls.torso, controls.elevation}, ratio);
}
void herc_body::prepare_player_movement(int16_t const throttle, int16_t const steering, frame_ratio const ratio, movement_event_sink const &events) {
  auto const notify{[&](movement_event const event){ frame_movement_events.push_back(event); if(events) events(event); }};
  if(update_player_movement(movement_state, source.profile.movement, rig.channel(body_channel), throttle, steering, damage, ratio, notify, expansion, direction_mode)) throttle_command_pending = true;
}
void herc_body::reset_aim_angles() { torso_state.angle = 0; elevation_state.angle = 0; }
void herc_body::align_disabled_player() {
  // Use the evaluated camera transform, including fixed-point angle extraction.
  // Adding the torso angle would miss animation offsets and quantisation.
  movement_state.angles.z = transform_angles(compose_transforms(aiming_pivot(), placement())).z;
  torso_state.angle = 0;
  if(torso_channel) rig.channel(*torso_channel).set_phase(source.profile.torso.sequence, 0);
  // 137c4 calls 215c(actor, 1): one unscaled time unit, not a redraw flag.
  // A negative cached playback rate can advance by -1 after the Q8 shift.
  advance_root_animation(movement_state, rig.channel(body_channel), 1);
  rig.update_pose();
}
void herc_body::stop_throttle() { movement_state.throttle = 0; throttle_command_pending = true; }
int16_t herc_body::exchange_throttle(int16_t const requested) {
  if(!throttle_command_pending) movement_state.throttle = requested;
  throttle_command_pending = false;
  return movement_state.throttle;
}

void herc_body::prepare_requested(int16_t const speed, int16_t const steering, frame_ratio const ratio, movement_event_sink const &events, movement_adjustment const &avoidance) {
  auto const notify{[&](movement_event const event){ frame_movement_events.push_back(event); if(events) events(event); }};
  update_requested_movement(movement_state, source.profile.movement, rig.channel(body_channel), speed, steering, damage, ratio, notify, avoidance, expansion);
}

void herc_body::prepare_aim(vec2<int16_t> const &aim, frame_ratio const ratio, std::optional<vec2<int16_t>> const target) {
  update_aim(torso_state, source.profile.torso, aim.x, ratio, target ? std::optional<int16_t>{target->x} : std::nullopt);
  if(sounds) sounds(servo_sound_request{0,aim.x,torso_state.angle});
  update_aim(elevation_state, source.profile.elevation, aim.y, ratio, target ? std::optional<int16_t>{target->y} : std::nullopt);
  if(sounds) sounds(servo_sound_request{1,aim.y,elevation_state.angle});
  if(torso_channel) rig.channel(*torso_channel).set_phase(source.profile.torso.sequence, static_cast<int16_t>(static_cast<uint16_t>(torso_state.angle) >> 2));
  if(elevation_channel) rig.channel(*elevation_channel).set_phase(source.profile.elevation.sequence, static_cast<int16_t>(static_cast<uint16_t>(elevation_state.angle) >> 2));
}

bool herc_body::step(herc_controls const &controls, int16_t const time_ratio, std::function<bool(herc_body const &)> const &contact, movement_event_sink const &events) {
  begin_frame();
  prepare_player(controls, time_ratio, events);
  return advance(time_ratio, contact);
}

bool herc_body::step_requested(int16_t const speed, int16_t const steering, vec2<int16_t> const &aim, int16_t const time_ratio, std::function<bool(herc_body const &)> const &contact, movement_event_sink const &events) {
  begin_frame();
  prepare_requested(speed, steering, time_ratio, events);
  prepare_aim(aim, time_ratio);
  return advance(time_ratio, contact);
}

model_transform herc_body::aiming_pivot() const {
  auto const *part{find_model_part(source.models.roots.front(), source.profile.camera_part)};
  if(!part || part->header.transform < 0) throw std::out_of_range{"HERC aiming pivot"};
  return rig.instance().transforms()[static_cast<size_t>(part->header.transform)];
}

bool herc_body::advance(frame_ratio const time_ratio, std::function<bool(herc_body const &)> const &contact, std::function<void()> const &blocked) {
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
  if(!contact || !contact(*this)){update_ground_attachments();return false;}
  restore();
  if(blocked) blocked();
  // 14844 / expansion 16758 also set the throttle ownership flag (+8d).
  // Otherwise the instrument exchange restores the stale slider value.
  stop_throttle();
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
  update_ground_attachments();
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

void herc_body::update_ground_attachments() {
  auto const world{placement()};
  for(size_t i{};i<ground.size();++i){
    if(!ground[i].active)continue;
    auto const *part{find_model_part(source.models.roots.front(),source.profile.attachment_parts[i])};
    if(!part||part->header.transform<0)continue;
    auto const &local{rig.instance().transforms()[static_cast<size_t>(part->header.transform)].translation};
    place_ground_attachment(ground[i],local,world,movement_state.angles.z);
  }
}

std::vector<uint16_t> herc_body::footstep_sounds() {
  std::vector<footstep_attachment> attachments;
  auto const &profile{source.profile}; auto const world{placement()};
  for(size_t i{};i<profile.attachment_types.size() && profile.attachment_types[i]>=0;++i) {
    auto const *part{find_model_part(source.models.roots.front(),profile.attachment_parts[i])};
    bool const active{ground[i].active && part && part->header.transform>=0};
    if(!active) { attachments.push_back({profile.attachment_types[i],false,0,0}); continue; }
    auto const &transform{rig.instance().transforms()[static_cast<size_t>(part->header.transform)]};
    attachments.push_back({profile.attachment_types[i],true,transform.translation.y,compose_transforms(transform,world).translation.z});
  }
  return advance_footsteps(footsteps,profile.movement,profile.footstep_trigger,profile.footstep_rearm,
                          rig.channel(body_channel).state(),movement_state.playback_rate,true,attachments);
}

} // namespace earthsiege
