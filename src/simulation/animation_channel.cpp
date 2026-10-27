#include "animation_channel.h"
#include <algorithm>
#include <stdexcept>
#include "fixed_math.h"
#include "model_instance.h"
#include "model_math.h"

namespace earthsiege {
animation_channel::animation_channel(animation_list const &animation) : source{animation} {
  reset(0);
}

void animation_channel::reset(animation_index const sequence, animation_index const frame, animation_time const elapsed) {
  /// Select a frame without discarding accumulated movement or playback rate
  auto const &definition{source.sequences.at(static_cast<size_t>(sequence))};
  current.duration = wrap_word(definition.frames.at(static_cast<size_t>(frame)).duration);
  current.sequence = current.next_sequence = sequence;
  current.frame = frame;
  current.next_frame = next(sequence, frame);
  current.elapsed = elapsed;
  frame_motion();
  current.requested_sequence = -1;
  current.stop_on_arrival = false;
  current.exit_frame = -1;
  current.flags = animation_channel_flags::arrived;
}

void animation_channel::request_sequence(animation_index const sequence, bool const stop_on_arrival) {
  /// Search authored exits in playback order; -1 cancels a pending request
  if(sequence < -1 || (sequence >= 0 && static_cast<size_t>(sequence) >= source.sequences.size())) throw std::out_of_range{"animation destination sequence"};
  current.requested_sequence = sequence;
  current.stop_on_arrival = stop_on_arrival;
  set_flag(animation_channel_flags::arrived, false);
  if(sequence == -1) {
    set_flag(animation_channel_flags::arrived, true);
    current.exit_frame = -1;
    return;
  }
  auto const &definition{source.sequences.at(static_cast<size_t>(current.sequence))};
  auto const start{current.rate < 0 ? current.frame : current.next_frame};
  current.exit_frame = start;
  do {
    auto const &frame{definition.frames.at(static_cast<size_t>(current.exit_frame))};
    for(unsigned int index{0}; index != frame.transition_count; ++index) {
      auto const &transition{source.transitions.at(frame.first_transition + index)};
      if(transition.destination_sequence != sequence) continue;
      current.transition_duration = wrap_word(transition.tick);
      current.destination_sequence = wrap_word(transition.destination_sequence);
      current.destination_frame = wrap_word(transition.destination_frame);
      current.transition_pose = wrap_word(transition.ground_movement);
      return;
    }
    auto const old{current.exit_frame};
    current.exit_frame = current.rate < 0 ? previous(current.sequence, old) : next(current.sequence, old);
    if(old == current.exit_frame) break;
  } while(current.exit_frame != start);
  current.exit_frame = -1;
}

void animation_channel::set_rate(animation_rate const rate) {
  /// Changing direction re-searches a queued transition from the current frame
  bool const direction_changed{(current.rate < 0) != (rate < 0)};
  current.rate = rate;
  if(direction_changed) request_sequence(current.requested_sequence, current.stop_on_arrival);
}

void animation_channel::advance(animation_time const delta) {
  /// Preserve signed-word timing, overshoot and transition movement ordering
  if(has_flag(animation_channel_flags::arrived) && current.stop_on_arrival) return;
  auto const step{wrap_word((static_cast<int>(delta) * current.rate) >> 8)};
  if(step == 0) return;
  auto elapsed{wrap_word(current.elapsed + step)};
  unsigned int remaining_steps{65'536};
  if(elapsed < 0 && current.frame == 0 && current.exit_frame != 0 && previous(current.sequence, 0) == 0) elapsed = 0;
  if(elapsed == 0) {
    current.elapsed = 0;
    if(has_flag(animation_channel_flags::transition)) {
      current.next_sequence = current.sequence;
      current.next_frame = next(current.sequence, current.frame);
      current.duration = wrap_word(source.sequences.at(static_cast<size_t>(current.sequence)).frames.at(static_cast<size_t>(current.frame)).duration);
      set_flag(animation_channel_flags::transition, false);
      frame_motion();
    }
    reached();
    return;
  }
  if(elapsed < 0) {
    do {
      if(--remaining_steps == 0) throw std::runtime_error{"animation reverse transition cycle does not advance time"};
      current.next_sequence = current.sequence;
      current.next_frame = current.frame;
      if(current.frame == current.exit_frame) {
        current.sequence = current.destination_sequence;
        current.frame = current.destination_frame;
        current.duration = current.transition_duration;
        current.exit_frame = -1;
        set_flag(animation_channel_flags::transition, true);
        load_motion(current.transition_pose);
      } else {
        current.frame = previous(current.sequence, current.frame);
        current.duration = wrap_word(source.sequences.at(static_cast<size_t>(current.sequence)).frames.at(static_cast<size_t>(current.frame)).duration);
        set_flag(animation_channel_flags::transition, false);
        frame_motion();
      }
      accumulate(true);
      if(-current.duration <= elapsed) {
        current.elapsed = wrap_word(current.duration + elapsed);
        elapsed = 0;
      } else {
        current.elapsed = 0;
        elapsed = wrap_word(elapsed + current.duration);
      }
      if(reached()) {
        elapsed = 0;
        current.elapsed = 0;
      }
    } while(elapsed < 0);
    return;
  }
  while(elapsed != 0 && elapsed >= current.duration) {
    if(--remaining_steps == 0) throw std::runtime_error{"animation forward transition cycle does not advance time"};
    accumulate(false);
    current.sequence = current.next_sequence;
    current.frame = current.next_frame;
    if(current.next_frame == current.exit_frame) {
      current.next_sequence = current.destination_sequence;
      current.next_frame = current.destination_frame;
      elapsed = wrap_word(elapsed - current.duration);
      current.duration = current.transition_duration;
      current.elapsed = 0;
      current.exit_frame = -1;
      set_flag(animation_channel_flags::transition, true);
      load_motion(current.transition_pose);
    } else {
      current.next_frame = next(current.next_sequence, current.next_frame);
      elapsed = wrap_word(elapsed - current.duration);
      current.duration = wrap_word(source.sequences.at(static_cast<size_t>(current.sequence)).frames.at(static_cast<size_t>(current.frame)).duration);
      if(current.duration == 0) elapsed = 0;
      current.elapsed = 0;
      set_flag(animation_channel_flags::transition, false);
      frame_motion();
    }
    if(reached()) elapsed = 0;
  }
  current.elapsed = elapsed;
}

void animation_channel::set_phase(animation_index const sequence, int16_t const phase) {
  /// Sample an angle-driven channel using the original Q14 phase mapping
  auto const &definition{source.sequences.at(static_cast<size_t>(sequence))};
  int total{0};
  for(auto const &frame : definition.frames) total += frame.duration;
  auto elapsed{wrap_word(((wrap_word(total) - 1) * static_cast<int>(phase) + matrix_rounding) >> matrix_fraction_bits)};
  size_t frame{0};
  while(definition.frames.at(frame).duration <= elapsed) {
    elapsed = wrap_word(elapsed - definition.frames[frame].duration);
    ++frame;
  }
  reset(sequence, wrap_word(static_cast<int>(frame)), elapsed);
}

animation_channel_state const &animation_channel::state() const {
  return current;
}

int16_t animation_channel::fraction() const {
  if(current.duration == 0) return 0;
  return wrap_word((current.duration / 2 + current.elapsed * animation_blend_one) / current.duration);
}

model_transform animation_channel::root_motion() const {
  /// Interpolate the unfinished step after the completed movement transforms
  if(!current.has_motion || current.duration == 0) return model_transform{current.accumulated};
  animation_pose part;
  for(unsigned int axis{0}; axis != 3; ++axis) {
    part.rotation[axis] = static_cast<uint16_t>(wrap_word(current.motion.rotation[axis]) * current.elapsed / current.duration);
    part.translation[axis] = wrap_word(current.motion.translation[axis] * current.elapsed / current.duration);
  }
  auto transform{rotation_transform(part.rotation)};
  transform.translation = vec3<int32_t>{part.translation};
  return compose_transforms(transform, current.accumulated);
}

void animation_channel::apply(model_instance &instance) const {
  instance.apply_frame(static_cast<unsigned int>(current.sequence), static_cast<unsigned int>(current.frame), static_cast<unsigned int>(current.next_sequence), static_cast<unsigned int>(current.next_frame), fraction());
}

animation_index animation_channel::next(animation_index const sequence, animation_index const frame) const {
  auto const &definition{source.sequences.at(static_cast<size_t>(sequence))};
  if(definition.frames.empty()) throw std::runtime_error{"animation sequence has no frames"};
  auto const count{static_cast<int>(definition.frames.size())};
  return wrap_word(definition.cyclic ? (frame + 1) % count : std::min(frame + 1, count - 1));
}

animation_index animation_channel::previous(animation_index const sequence, animation_index const frame) const {
  auto const &definition{source.sequences.at(static_cast<size_t>(sequence))};
  if(definition.frames.empty()) throw std::runtime_error{"animation sequence has no frames"};
  auto const count{static_cast<int>(definition.frames.size())};
  return wrap_word(definition.cyclic ? (frame + count - 1) % count : std::max(frame - 1, 0));
}

void animation_channel::load_motion(animation_index const pose) {
  current.motion = source.poses.at(static_cast<size_t>(pose));
  current.has_motion = true;
}

void animation_channel::frame_motion() {
  auto const &definition{source.sequences.at(static_cast<size_t>(current.sequence))};
  if(definition.ground_movement) {
    load_motion(wrap_word(definition.pose_indices.at(static_cast<size_t>(current.frame) * definition.parts.size())));
  } else {
    current.has_motion = false;
  }
}

void animation_channel::accumulate(bool const reverse) {
  if(!current.has_motion) return;
  auto transform{rotation_transform(current.motion.rotation)};
  transform.translation = vec3<int32_t>{current.motion.translation};
  if(reverse) {
    for(unsigned int column{0}; column != 3; ++column) {
      for(unsigned int row{column + 1}; row != 3; ++row) std::swap(transform.basis.at(column, row), transform.basis.at(row, column));
    }
    transform.translation = rotate_vector(-transform.translation, transform);
  }
  current.accumulated = compose_transforms(transform, current.accumulated);
}

bool animation_channel::reached() {
  if(current.sequence != current.requested_sequence) return false;
  set_flag(animation_channel_flags::arrived, true);
  return current.stop_on_arrival;
}

bool animation_channel::has_flag(animation_channel_flags const flag) const {
  return (static_cast<uint16_t>(current.flags) & static_cast<uint16_t>(flag)) != 0;
}
void animation_channel::set_flag(animation_channel_flags const flag, bool const value) {
  auto const bits{static_cast<uint16_t>(current.flags)};
  auto const mask{static_cast<uint16_t>(flag)};
  current.flags = static_cast<animation_channel_flags>(value ? bits | mask : bits & ~mask);
}

} // namespace earthsiege
