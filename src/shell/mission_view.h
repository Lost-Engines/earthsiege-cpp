#pragma once
#include <memory>
#include "render/scenery_renderer.h"
#include "simulation/keyboard_input.h"
#include "simulation/mission_session.h"

namespace earthsiege {

/// Inspect original demo starting states while full mission consumers are integrated
class mission_view {
  std::unique_ptr<mission_session> session;
  framebuffer frame;
  scenery_renderer renderer;
  camera_state camera;
  keyboard_input keyboard;
  bool paused{false};
  bool enter_held{false};

public:
  mission_view(resource_store const &resources, unsigned int demo);
  void key(held_key key, bool pressed, bool alt);
  void clear_input();
  void toggle_pause();
  void change_camera(camera_mode mode);
  void step(bool control, bool zoom);
  void draw();
  framebuffer const &image() const;
  palette const &colours() const;
};

} // namespace earthsiege
