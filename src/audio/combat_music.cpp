#include "combat_music.h"
#include <algorithm>
#include <cstddef>

namespace earthsiege {

std::vector<music_request> advance_combat_music(combat_music_state &state, music_playback &playback, std::span<music_contact const> const actors,
                                                std::span<music_contact const> const flyers, uint32_t const clock, bool const enabled,
                                                bool const driver, random_state &random) {
  /// Select a track and advance fades on the simulation clock
  std::vector<music_request> requests;
  if(!enabled || !driver) return requests;

  int16_t constexpr tracks_per_mode{3};
  uint32_t constexpr check_interval{300};

  auto const request{[&state, &requests](music_action const action, uint8_t const value = 0){
    requests.push_back({
      .action{action},
      .track{static_cast<uint8_t>(state.track)},
      .value{value},
    });
  }};

  if(state.track == -1) {
    state.track = random.bounded(tracks_per_mode);
    request(music_action::load);
    state.battle = false;
    state.deadline = clock + check_interval;
  }

  if(state.fading) {
    auto const previous_track{static_cast<size_t>(state.track)};
    auto &volume{playback.volume.at(previous_track)};
    if(volume) {
      volume = static_cast<uint8_t>(std::max(0, static_cast<int>(volume) - 15));
      request(music_action::volume, volume);
      return requests;
    }

    playback.playing[previous_track] = false;
    request(music_action::stop);
    state.track = static_cast<int16_t>(state.track + (state.battle ? tracks_per_mode : -tracks_per_mode));
    request(music_action::load);
    playback.volume.at(static_cast<size_t>(state.track)) = 100;
    request(music_action::volume, 100);
    state.fading = false;
  }

  if(state.deadline < clock) {
    for(auto const &actor : actors) {
      if(actor.disabled || actor.dead || actor.waiting || actor.side != 1) continue;
      if(actor.distance >= 25'000 && !actor.visible) continue;
      if(state.battle) return requests;

      state.fading = true;
      state.battle = true;
      state.deadline = clock + check_interval;
      break;
    }

    if(state.battle && !state.fading) {
      // the original distant-flyer test considers both sides
      state.fading = std::ranges::none_of(flyers, [](music_contact const &flyer){
        return !flyer.dead && !flyer.disabled && flyer.distance > 18'928;
      });
      if(state.fading) {
        state.battle = false;
        state.deadline = clock + check_interval;
      }
    }
  }

  auto const track{static_cast<size_t>(state.track)};
  if(!playback.playing.at(track)) {
    playback.playing[track] = true;
    request(music_action::play);
  }
  return requests;
}

} // namespace earthsiege
