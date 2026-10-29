#include "effect_state.h"
#include <algorithm>
#include <stdexcept>
#include "simulation/fixed_math.h"
#include "resources/combat.h"
#include "simulation/model_math.h"

namespace earthsiege {

smoke_emitter_state create_smoke_emitter(int16_t const family, random_state &random) {
  if(family < 0 || family > 1) throw std::out_of_range{"smoke family has no supplied model"};
  return {family, static_cast<int16_t>(family == 0 ? 200 : 500), 0, static_cast<int16_t>(family * 4 + (random.next() & 3))};
}
std::optional<int16_t> advance_smoke_emitter(smoke_emitter_state &state, frame_ratio const ratio) {
  if(!state.remaining) return {};
  state.timer = std::max<int16_t>(0, wrap_word(state.timer - ratio));
  if(state.timer) return {};
  state.timer = 200; --state.remaining;
  auto const root{state.root}; state.root = static_cast<int16_t>(state.family * 4 + ((state.root + 1) & 3));
  return root;
}
bool advance_smoke_puff(smoke_puff_state &state, uint16_t const frames, frame_ratio const ratio) {
  for(unsigned int axis{}; axis < 3; ++axis) state.position[axis] = wrap_long(static_cast<int64_t>(state.position[axis]) + state.velocity[axis]);
  state.timer = std::max(0, wrap_long(static_cast<int64_t>(state.timer) - ratio));
  if(state.timer) return false;
  state.timer = 200; state.cell = static_cast<uint16_t>((state.cell + 1U) % frames);
  return state.cell == 0;
}
vec3<int32_t> smoke_attachment_position(model_transform const &owner, std::optional<vec3<int16_t>> const &offset, model_transform const *part) {
  if(!part && !offset) return vec3<int32_t>{owner.translation};
  auto local{part ? (offset ? transform_point(*offset, *part) : vec3<int32_t>{part->translation}) : vec3<int32_t>{*offset}};
  auto result{rotate_vector(local, owner)};
  for(unsigned int axis{}; axis < 3; ++axis) result[axis] = wrap_long(static_cast<int64_t>(result[axis]) + owner.translation[axis]);
  return result;
}
debris_particle create_detached_weapon(weapon_definition const &definition, weapon_mount const &mount, model_transform const &hardpoint, vec3<int32_t> const &owner_centre) {
  auto const position{transform_point(weapon_mount_offset(mount, definition), hardpoint)};
  auto const heading{bearing_angle(owner_centre, position)};
  auto const elevation{static_cast<uint16_t>(mount.source[24] | (mount.source[25] << 8))};
  auto const horizontal{wrap_word((400 * model_cosine(elevation) + matrix_rounding) >> matrix_fraction_bits)};
  vec3<int16_t> const velocity{
    wrap_word((horizontal * model_cosine(static_cast<uint16_t>(heading + angle_quarter_turn)) + matrix_rounding) >> matrix_fraction_bits),
    wrap_word((horizontal * model_cosine(heading) + matrix_rounding) >> matrix_fraction_bits),
    wrap_word((400 * model_cosine(static_cast<uint16_t>(elevation - angle_quarter_turn)) + matrix_rounding) >> matrix_fraction_bits)};
  return {definition.model_indices[2], vec3<int32_t>{position}, transform_angles(hardpoint), vec3<int16_t>{velocity}};
}

explosion_state create_explosion(explosion_definition const &definition, size_t const cells, vec3<int32_t> const &position, random_state &random) {
  int16_t constexpr explosion_sound_first{10};
  explosion_state result;
  result.position = position;
  result.angles = {0, static_cast<uint16_t>(random.next()), 0};
  result.remaining = definition.animation_period;
  result.cells.resize(cells);
  if(definition.secondary) result.secondary_cell = 0;
  if(definition.light != effect_light_kind::none) result.light_intensity = definition.light_curve.front();
  result.sound = wrap_word(definition.sound + explosion_sound_first);
  return result;
}

bool advance_explosion(explosion_state &state, explosion_definition const &definition, std::span<int16_t const> const frame_counts, uint16_t const secondary_count, frame_ratio const ratio) {
  state.remaining = std::max<int16_t>(0, wrap_word(state.remaining - ratio));
  if(state.remaining != 0) return false;
  if(definition.cell_sequence < 0) return true;
  auto const cell{static_cast<size_t>(definition.cell_sequence)};
  if(cell >= frame_counts.size() || cell >= state.cells.size() || frame_counts[cell] <= 0) throw std::out_of_range{"explosion cell sequence"};
  auto &frame{state.cells[cell]};
  frame = static_cast<uint16_t>((frame + 1U) % static_cast<unsigned int>(frame_counts[cell]));
  if(frame == 0) return true;
  if(state.light_intensity) state.light_intensity = definition.frame_light(frame);
  if(state.secondary_cell) {
    if(secondary_count == 0) throw std::invalid_argument{"zero secondary explosion frame count"};
    *state.secondary_cell = static_cast<uint16_t>((*state.secondary_cell + 1U) % secondary_count);
  }
  state.remaining = definition.animation_period;
  return false;
}

bool advance_debris(debris_particle &state) {
  int constexpr spin{1500}, gravity{50}, drag_q10{50}, bounce_q10{450}, minimum_bounce{100};
  state.angles.x = static_cast<uint16_t>(state.angles.x + spin);
  state.velocity.z = wrap_word(state.velocity.z - gravity);
  for(unsigned int axis{0}; axis != 2; ++axis) state.velocity[axis] = wrap_word(state.velocity[axis] - wrap_word((drag_q10 * state.velocity[axis] + 512) >> 10));
  for(unsigned int axis{0}; axis != 3; ++axis) state.position[axis] = wrap_long(static_cast<int64_t>(state.position[axis]) + state.velocity[axis]);
  if(state.position.z >= 0) return false;
  state.position.z = 0;                                                        // original debris uses the flat plane, not scenery height fields
  state.velocity.z = wrap_word(-wrap_word((bounce_q10 * state.velocity.z + 512) >> 10));
  return state.velocity.z < minimum_bounce;
}

std::vector<debris_particle> create_debris_burst(debris_definition const &definition, vec3<int32_t> const &position, random_state &random) {
  if(definition.count_limit <= 0 || definition.total_weight <= 0 || definition.models.empty()) throw std::invalid_argument{"invalid debris choice range"};
  auto const count{random.bounded(definition.count_limit)};
  std::vector<debris_particle> particles;
  particles.reserve(static_cast<size_t>(count));
  for(int i{0}; i != count; ++i) {
    auto choice{random.bounded(definition.total_weight)};
    size_t index{0};
    // Equality stays in the preceding bucket in the original weighted selector.
    while(definition.models.at(index).weight < choice) { choice = wrap_word(choice - definition.models[index].weight); ++index; }
    int16_t constexpr elevation_range{24000}, elevation_origin{-10000}, launch_speed{400};
    auto const elevation{wrap_word(random.bounded(elevation_range) + elevation_origin)};
    auto const heading{static_cast<uint16_t>(random.next())};
    auto const horizontal{wrap_word((launch_speed * model_cosine(static_cast<uint16_t>(elevation)) + matrix_rounding) >> matrix_fraction_bits)};
    vec3<int16_t> const velocity{
      wrap_word((horizontal * model_cosine(static_cast<uint16_t>(heading + angle_quarter_turn)) + matrix_rounding) >> matrix_fraction_bits),
      wrap_word((horizontal * model_cosine(heading) + matrix_rounding) >> matrix_fraction_bits),
      wrap_word((launch_speed * model_cosine(static_cast<uint16_t>(elevation - angle_quarter_turn)) + matrix_rounding) >> matrix_fraction_bits),
    };
    particles.push_back({definition.models[index].model, vec3<int32_t>{position}, {0, 0, heading}, vec3<int16_t>{velocity}});
  }
  return particles;
}

} // namespace earthsiege
