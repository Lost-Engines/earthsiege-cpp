#pragma once
#include <deque>
#include <memory>
#include "render/scenery_renderer.h"
#include "render/instrument_bars.h"
#include "render/shield_instruments.h"
#include "resources/cockpit.h"
#include "simulation/keyboard_input.h"
#include "shell/pointer_controls.h"
#include "simulation/mission_session.h"

namespace earthsiege {

/// Inspect original demo starting states while full mission consumers are integrated
class mission_view {
  std::unique_ptr<mission_session> session;
  framebuffer frame;
  cockpit_artwork cockpit;
  reactor_gauge reactor;
  throttle_gauge throttle;
  shield_instruments shields;
  cockpit_view panel{cockpit_view::forward};
  palette display_colours;
  scenery_renderer renderer;
  camera_state camera;
  keyboard_input keyboard;
  bool paused{false};
  player_input pointer;
  pointer_controls instruments;
  std::deque<player_input> pointer_events;
  int16_t requested_throttle{};
  std::optional<bool> shield_to_front;
  void consume_pointer_events();
  std::deque<uint16_t> commands;

public:
  mission_view(resource_store const &resources, unsigned int demo);
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
  framebuffer const &image() const;
  palette const &colours() const;
};

} // namespace earthsiege
