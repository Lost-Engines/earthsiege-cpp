#include "effect_state.h"
#include <algorithm>
#include <stdexcept>
#include "simulation/fixed_math.h"
#include "simulation/model_math.h"

namespace earthsiege {

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
