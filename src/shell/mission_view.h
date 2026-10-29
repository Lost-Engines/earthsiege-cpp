#pragma once
#include <deque>
#include "audio/mission_audio.h"
#include <memory>
#include "render/scenery_renderer.h"
#include "render/clouds.h"
#include "render/ground_marks.h"
#include "render/instrument_bars.h"
#include "render/shield_instruments.h"
#include "render/weapon_instruments.h"
#include "render/navigation_instruments.h"
#include "render/clock_instrument.h"
#include "render/presentation_player.h"
#include "resources/cockpit.h"
#include "simulation/keyboard_input.h"
#include "shell/pointer_controls.h"
#include "shell/cockpit_displays.h"
#include "shell/cockpit_radio.h"
#include "shell/cockpit_hdd.h"
#include "simulation/mission_session.h"

namespace earthsiege {

/// Inspect original demo starting states while full mission consumers are integrated
class mission_view {
  std::unique_ptr<mission_session> session;
  random_state &ui_random;
  mission_audio audio;
  framebuffer frame;
  cockpit_artwork cockpit;
  reactor_gauge reactor;
  throttle_gauge throttle;
  shield_instruments shields;
  weapon_instruments weapons;
  navigation_instruments navigation;
  waypoint_instruments waypoints;
  clock_instrument clock;
  presentation_player lamps;
  uint64_t elapsed_milliseconds{};
  uint32_t interface_ticks{};
  struct weapon_control_binding {
    size_t row;
    size_t button;
    std::optional<size_t> slider;
    int32_t requested_charge;
  };
  std::vector<weapon_control_binding> weapon_controls;
  std::array<size_t, 5> weapon_groups;
  cockpit_view panel{cockpit_view::forward};
  palette display_colours;
  std::vector<bitmap> horizon_images;
  bool panorama_enabled{true};
  uint8_t cloud_detail{255},ground_detail{255};
  cloud_field clouds;
  ground_marks marks;
  scenery_renderer renderer;
  camera_state camera;
  keyboard_input keyboard;
  bool paused{false};
  player_input pointer;
  pointer_controls instruments;
  std::unique_ptr<cockpit_displays> displays;
  std::unique_ptr<cockpit_radio> radio;
  std::unique_ptr<cockpit_hdd> hdd;
  std::deque<player_input> pointer_events;
  int16_t requested_throttle{};
  std::optional<bool> shield_to_front;
  void consume_pointer_events();
  std::deque<uint16_t> commands;

public:
  mission_view(resource_store const &resources, unsigned int demo);
  mission_view(resource_store const &resources, std::unique_ptr<mission_session> mission);
  mission_results results() const { return session->results(); }
  mission_counters const &mission_variables() const { return session->mission_variables(); }
  mission_status status(bool requested=false) const { return session->mission_state(requested); }
  std::optional<mission_status> take_mission_prompt() { return session->take_mission_prompt(); }
  void key(held_key key, bool pressed, bool alt);
  void command(uint16_t code);
  void mouse_position(int x, int y);
  void mouse_button(bool right, bool pressed);
  void clear_input();
  void toggle_pause();
  void change_camera(camera_mode mode);
  void change_cockpit_view(cockpit_view view);
  void step(bool control, bool zoom);
  void draw();
  void environment_detail(bool horizon,uint8_t cloud,uint8_t ground) { panorama_enabled=horizon;cloud_detail=cloud;ground_detail=ground; }
  std::vector<int16_t> take_audio();
  std::optional<vec2<int32_t>> take_pointer_warp();
  framebuffer const &image() const;
  palette const &colours() const;
};

} // namespace earthsiege
