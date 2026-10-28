#include "herc_sensors.h"
#include <algorithm>
#include "simulation/fixed_math.h"
#include "simulation/model_math.h"

namespace earthsiege {
namespace {
int16_t tick(int16_t &timer, frame_ratio ratio) { timer = std::max<int16_t>(0, wrap_word(timer - ratio)); return timer; }
}
void advance_herc_sensors(herc_sensor_state &state, herc_impact_state &impact, bool &target_changed, herc_sensor_inputs const &input, frame_ratio const ratio, random_state &random, std::function<void()> const &notify_target, std::function<void()> const &update_hud) {
  tick(impact.ecm_remaining, ratio);
  if(tick(state.damage_window_remaining, ratio) == 0) { state.damage_window_remaining = 4000; impact.damage_received = 0; }
  if(input.target && input.ecm_pulse && notify_target) notify_target();
  state.ecm_active = input.ecm_active;
  state.locks.fill(false);
  state.hud_locked = false;
  if(!input.target) return;
  auto const &target{*input.target};
  bool new_interference{false};
  if(target.herc && target.ecm_active) {
    if(tick(state.interference_remaining, ratio) == 0) {
      new_interference = (random.next() & 4095) < 820;
      state.interfered = new_interference;
      state.interference_remaining = static_cast<int16_t>(new_interference ? 5000 : 1500);
    }
  } else state.interfered = false;
  vec3<int32_t> delta;
  for(unsigned int axis{0}; axis != 3; ++axis) delta[axis] = wrap_long(static_cast<int64_t>(target.position[axis]) - input.position[axis]);
  auto const acquisition_time{wrap_word(std::min(32767, fast_distance(delta) >> 2))};
  auto const error{wrap_word(bearing_angle(input.position, target.position) - input.heading + input.torso)};
  bool const outside_gate{target_changed || error < -12288 || error >= 12288};
  if(!outside_gate) {
    for(size_t const channel : {0U, 4U}) if(input.missile_counts[channel] != 0) {
      if(!impact.detectable || new_interference) state.acquisition_remaining[channel] = acquisition_time;
      else if(tick(state.acquisition_remaining[channel], ratio) == 0 && !state.interfered) state.locks[channel] = true;
    }
    if(input.missile_counts[1] != 0) {
      if(new_interference) state.acquisition_remaining[1] = acquisition_time;
      else if(tick(state.acquisition_remaining[1], ratio) == 0 && !state.interfered) state.locks[1] = true;
    }
    if(input.missile_counts[2] != 0) {
      if(!target.detectable && !target.ecm_active) state.acquisition_remaining[2] = acquisition_time;
      else if(tick(state.acquisition_remaining[2], ratio) == 0) state.locks[2] = true;
    }
  } else {
    // Original resets only channels 0..2 here. Manual guidance (3) has no lock,
    // and channel 4 deliberately keeps its existing countdown across this gate.
    std::fill_n(state.acquisition_remaining.begin(), 3, acquisition_time);
  }
  if(input.player) {
    state.hud_locked = input.selected_channel ? state.locks.at(*input.selected_channel) : std::ranges::any_of(state.locks, [](bool value){return value;});
    if(update_hud) update_hud();
  }
  if(outside_gate) target_changed = false;
}
} // namespace earthsiege
