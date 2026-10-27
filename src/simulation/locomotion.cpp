#include "locomotion.h"
#include <algorithm>
#include <cstdlib>
#include <stdexcept>
#include "simulation/fixed_math.h"
#include "simulation/model_math.h"

namespace earthsiege {
namespace {

int16_t scale(int const value, int const rate, unsigned int const bits = 8) {
  return wrap_word((value * rate + (1 << (bits - 1))) >> bits);
}

int16_t approach(int16_t const value, int16_t const desired, int16_t const step) {
  if(desired < value) return std::max(desired, wrap_word(value - step));
  if(desired > value) return std::min(desired, wrap_word(value + step));
  return value;
}

int constexpr throttle_limit{1024};
int constexpr throttle_change_scale{145};
int constexpr steering_axis_limit{256};
int constexpr throttle_direction_threshold{60};
int constexpr stationary_steering_threshold{51};
int constexpr steering_gait_threshold{45};
int16_t constexpr minimum_start_rate{60};
int16_t constexpr stop_transition_rate{100};
int constexpr turn_animation_rate{350};

} // anonymous namespace

void update_player_movement(locomotion_state &state, movement_profile const &profile, animation_channel &channel, int16_t const throttle_input, int16_t steering_input) {
  /// Preserve per-update throttle and acceleration; only root-motion time is scaled
  auto const &pose{channel.state()};
  bool const forward{std::abs(throttle_input) > throttle_direction_threshold ? throttle_input <= 0 : state.throttle >= 0};
  if(forward) steering_input = wrap_word(-steering_input);
  auto const throttle_change{scale(-throttle_input, throttle_change_scale)};
  if(throttle_change != 0) {
    auto const next_throttle{wrap_word(state.throttle + throttle_change)};
    bool const crosses_neutral{(state.throttle & 4095) != 0 && (next_throttle < 0) != (state.throttle < 0)};
    state.throttle = crosses_neutral ? 0 : static_cast<int16_t>(std::clamp<int>(next_throttle, -throttle_limit, throttle_limit));
  }
  auto const desired{scale(state.throttle, state.throttle < 0 ? -profile.reverse_rate : profile.forward_rate, 10)};
  auto const previous{state.gait_rate};
  state.gait_rate = approach(state.gait_rate, std::clamp(desired, profile.reverse_rate, profile.forward_rate), profile.acceleration);
  steering_input = static_cast<int16_t>(std::clamp<int>(steering_input, -steering_axis_limit, steering_axis_limit));
  bool const turning{pose.sequence == profile.turn_sequence};
  bool const stopping{pose.sequence == profile.forward_stop_sequence || pose.sequence == profile.reverse_stop_sequence};
  bool transition_controls_steering{false};
  auto magnitude{std::abs(state.gait_rate)};
  auto const previous_magnitude{std::abs(previous)};
  if(magnitude != 0 && turning && !stopping) {
    steering_input = 0;
    magnitude = 0;
    state.gait_rate = 0;
  }
  if(pose.sequence == pose.next_sequence) {
    if(previous_magnitude < magnitude) {
      if(!stopping) {
        if(pose.sequence == profile.movement_sequence) {
          state.playback_rate = state.gait_rate;
          if(pose.requested_sequence == profile.forward_stop_sequence || pose.requested_sequence == profile.reverse_stop_sequence) channel.cancel_request();
          if(state.gait_rate > profile.gait_threshold && pose.requested_sequence != profile.fast_sequence) channel.request_sequence(profile.fast_sequence);
        } else channel.cancel_request();
      } else {
        if(state.gait_rate < 1) {
          state.gait_rate = std::max(state.gait_rate, wrap_word(-profile.gait_threshold));
          state.playback_rate = std::min(state.gait_rate, wrap_word(-minimum_start_rate));
        } else {
          state.gait_rate = std::min(state.gait_rate, profile.gait_threshold);
          state.playback_rate = std::max(state.gait_rate, minimum_start_rate);
        }
        transition_controls_steering = true;
        if(pose.requested_sequence != profile.movement_sequence) {
          channel.reset(state.gait_rate < 0 ? profile.reverse_stop_sequence : profile.forward_stop_sequence);
          channel.request_sequence(profile.movement_sequence);
        }
      }
    } else if(magnitude < previous_magnitude) {
      if(pose.sequence == profile.fast_sequence) {
        if(magnitude < profile.gait_threshold) {
          state.gait_rate = previous < 1 ? profile.reverse_fast_exit_rate : profile.gait_threshold;
          state.playback_rate = state.gait_rate;
          if(pose.requested_sequence != profile.movement_sequence) channel.request_sequence(profile.movement_sequence);
        } else channel.cancel_request();
      } else if(pose.sequence == profile.movement_sequence) {
        if(magnitude == 0) {
          state.playback_rate = 0;
          if(!stopping) {
            auto const destination{previous < 1 ? profile.reverse_stop_sequence : profile.forward_stop_sequence};
            if(pose.requested_sequence != destination) channel.request_sequence(destination);
            state.playback_rate = previous < 1 ? -stop_transition_rate : stop_transition_rate;
            transition_controls_steering = true;
          }
        } else if(pose.requested_sequence == profile.fast_sequence) channel.cancel_request();
      }
    } else if(magnitude == 0) {
      if(std::abs(steering_input) < stationary_steering_threshold || !(stopping || turning)) {
        if(turning && !stopping) {
          if(pose.requested_sequence != profile.forward_stop_sequence) channel.request_sequence(profile.forward_stop_sequence);
          state.playback_rate = state.playback_rate > 0 ? stop_transition_rate : -stop_transition_rate;
        }
      } else if(!turning) {
        if(pose.requested_sequence != profile.turn_sequence) {
          channel.reset(steering_input < 0 ? profile.reverse_stop_sequence : profile.forward_stop_sequence);
          channel.request_sequence(profile.turn_sequence);
        }
        state.playback_rate = steering_input < 0 ? -stop_transition_rate : stop_transition_rate;
      } else state.playback_rate = scale(turn_animation_rate, steering_input, 10);
    } else state.playback_rate = state.gait_rate;
  } else state.gait_rate = previous;

  int steering{0};
  int const absolute{std::abs(state.gait_rate)};
  if(!transition_controls_steering && absolute >= steering_gait_threshold) {
    int const span{profile.forward_rate - steering_gait_threshold};
    int const half{span >> 1};
    int const amount{absolute - steering_gait_threshold};
    steering = amount > half ? profile.steering_rate - profile.steering_rate * (amount - half) / ((span - half) * 2) : profile.steering_rate * amount / half;
  }
  state.steering_rate = approach(state.steering_rate, scale(steering_input, steering), profile.steering_acceleration);
  state.angles.z = static_cast<uint16_t>(state.angles.z + state.steering_rate);
}

void advance_root_movement(locomotion_state &state, animation_channel &channel, int16_t const frame_ratio) {
  channel.set_rate(state.playback_rate);
  channel.rebase_motion();
  int constexpr nominal_step{100};
  channel.advance(std::max<int16_t>(-32767, scale(nominal_step, frame_ratio)));
  auto const movement{channel.root_motion()};
  auto const rotation{rotation_transform(state.angles)};
  auto const displacement{rotate_vector(movement.translation, rotation)};
  for(unsigned int axis{0}; axis != 3; ++axis) state.position[axis] = wrap_long(static_cast<int64_t>(state.position[axis]) + displacement[axis]);
  if(movement.basis.data[2] != 0 || movement.basis.data[5] != 0) throw std::runtime_error{"non-planar root motion requires the general actor orientation consumer"};
  state.angles.z = static_cast<uint16_t>(state.angles.z + direction_angle(movement.basis.data[4], -movement.basis.data[3]));
}

} // namespace earthsiege
